/*
 * eepromStore.cpp - se eepromStore.hpp och docs/eeprom-layout.md.
 */
#include "eepromStore.hpp"
#include "crc16.hpp"

namespace EepromLayout {

/* Sidstorlekar i flash (alla < 256), sa att tabellen inte tar RAM pa AVR. */
static const uint8_t pageSizes[PageCount] SC_PROGMEM =
{
    (uint8_t)Page1Size, (uint8_t)Page2Size, (uint8_t)Page3Size, (uint8_t)Page4Size, (uint8_t)Page5Size
};

uint16_t pageSize(uint8_t page)
{
    return (page < PageCount) ? (uint16_t)scReadFlashByte(&pageSizes[page]) : 0U;
}

uint16_t pageRamOffset(uint8_t page)
{
    uint16_t off = 0U;
    if (page < PageCount)
    {
        for (uint8_t i = 0U; i < page; ++i)
        {
            off = (uint16_t)(off + pageSize(i));
        }
    }
    return off;
}

uint16_t pageAddress(uint8_t page)
{
    return (page < PageCount)
        ? (uint16_t)(Page1Address + pageRamOffset(page) + ((uint16_t)page * CrcSize))
        : 0U;
}

} /* namespace EepromLayout */

namespace {
const uint8_t DiagPageIndex = 2U; /* sida 3 */
}

EepromStore::EepromStore(const EepromBackend& backend)
    : backend_(backend), warn_(0U)
{
    guard_.chargingActive = 0;
    guard_.outputOff = 0;
    guard_.outputRestore = 0;
    guard_.ctx = 0;
    loadDefaults();
}

void EepromStore::setBurnGuard(const BurnGuard& guard)
{
    guard_ = guard;
}

bool EepromStore::backendOk() const
{
    return (backend_.read != 0) && (backend_.update != 0);
}

bool EepromStore::guardOk() const
{
    return (guard_.chargingActive != 0) && (guard_.outputOff != 0);
}

void EepromStore::loadDefaults()
{
    for (uint16_t i = 0U; i < EepromLayout::PagesTotalSize; ++i)
    {
        data_[i] = scReadFlashByte(&eepromDefaults[i]);
    }
}

void EepromStore::defaultsForPage(uint8_t index)
{
    const uint16_t base = EepromLayout::pageRamOffset(index);
    const uint16_t size = EepromLayout::pageSize(index);
    for (uint16_t i = 0U; i < size; ++i)
    {
        data_[base + i] = scReadFlashByte(&eepromDefaults[base + i]);
    }
}

void EepromStore::forceDiagNormal()
{
    const uint16_t pos = EepromLayout::Page3RamOffset + EepromLayout::P3DiagModeOffset;
    data_[pos] = (uint8_t)(data_[pos] & (uint8_t)(~EepromLayout::P3DiagModeMask));
}

bool EepromStore::headerValid() const
{
    uint8_t h[EepromLayout::HeaderSize];
    backend_.read(EepromLayout::BaseAddress, h, EepromLayout::HeaderSize, backend_.ctx);
    const uint16_t crc = crc16(h, EepromLayout::HeaderDataSize);
    return (h[0] == EepromLayout::Magic0)
        && (h[1] == EepromLayout::Magic1)
        && (h[2] == EepromLayout::LayoutVersion)
        && (h[3] == EepromLayout::PageCount)
        && (scGetU16le(&h[EepromLayout::HeaderDataSize]) == crc);
}

bool EepromStore::loadPage(uint8_t index)
{
    const uint16_t base = EepromLayout::pageRamOffset(index);
    const uint16_t size = EepromLayout::pageSize(index);
    const uint16_t addr = EepromLayout::pageAddress(index);
    uint8_t crcBytes[EepromLayout::CrcSize];

    backend_.read(addr, &data_[base], size, backend_.ctx);
    backend_.read((uint16_t)(addr + size), crcBytes, EepromLayout::CrcSize, backend_.ctx);
    return crc16(&data_[base], size) == scGetU16le(crcBytes);
}

uint8_t EepromStore::load()
{
    warn_ = 0U;
    if (!backendOk())
    {
        loadDefaults();
        warn_ = (uint8_t)(WarnNoBackend | WarnHeaderInvalid);
    }
    else if (!headerValid())
    {
        /* Tomt EEPROM, B-2-firmware (som aldrig sparade) eller annan layout.
         * Gamla data tolkas aldrig med den nya layouten. */
        loadDefaults();
        warn_ = WarnHeaderInvalid;
    }
    else
    {
        for (uint8_t p = 0U; p < EepromLayout::PageCount; ++p)
        {
            if (!loadPage(p))
            {
                defaultsForPage(p);
                warn_ = (uint8_t)(warn_ | (uint8_t)(WarnPage1Crc << p));
            }
        }
    }
    forceDiagNormal();
    return warn_;
}

/* Byte som den ska lagras i EEPROM (diagMode maskas till Normal). */
uint8_t EepromStore::storedByte(uint8_t index, uint16_t offset) const
{
    uint8_t b = data_[EepromLayout::pageRamOffset(index) + offset];
    if ((index == DiagPageIndex) && (offset == EepromLayout::P3DiagModeOffset))
    {
        b = (uint8_t)(b & (uint8_t)(~EepromLayout::P3DiagModeMask));
    }
    return b;
}

uint16_t EepromStore::storedCrc(uint8_t index) const
{
    uint16_t crc = Crc16Init;
    const uint16_t size = EepromLayout::pageSize(index);
    for (uint16_t i = 0U; i < size; ++i)
    {
        crc = crc16Update(crc, storedByte(index, i));
    }
    return crc;
}

EepromStore::BurnResult EepromStore::burn()
{
    if (!backendOk())
    {
        warn_ = (uint8_t)(warn_ | WarnNoBackend | WarnBurnFailed);
        return BurnErrorNoBackend;
    }
    if (!guardOk())
    {
        return BurnRejectedNoGuard;
    }
    if (guard_.chargingActive(guard_.ctx))
    {
        return BurnRejectedCharging;
    }

    guard_.outputOff(guard_.ctx);
    const bool ok = writeAndVerify();
    if (guard_.outputRestore != 0)
    {
        guard_.outputRestore(guard_.ctx);
    }

    /* Efter lyckad burn stammer EEPROM med RAM: gamla lastvarningar nollas. */
    warn_ = ok ? 0U : (uint8_t)(warn_ | WarnBurnFailed);
    return ok ? BurnOk : BurnErrorVerify;
}

bool EepromStore::writeAndVerify()
{

    /* 1) Sidorna (data + CRC). update skriver bara andrade byte. */
    for (uint8_t p = 0U; p < EepromLayout::PageCount; ++p)
    {
        const uint16_t base = EepromLayout::pageRamOffset(p);
        const uint16_t size = EepromLayout::pageSize(p);
        const uint16_t addr = EepromLayout::pageAddress(p);
        uint8_t crcBytes[EepromLayout::CrcSize];

        if (p == DiagPageIndex)
        {
            const uint16_t d = EepromLayout::P3DiagModeOffset;
            const uint8_t diagByte = storedByte(p, d);
            backend_.update(addr, &data_[base], d, backend_.ctx);
            backend_.update((uint16_t)(addr + d), &diagByte, 1U, backend_.ctx);
            backend_.update((uint16_t)(addr + d + 1U), &data_[base + d + 1U],
                            (uint16_t)(size - d - 1U), backend_.ctx);
        }
        else
        {
            backend_.update(addr, &data_[base], size, backend_.ctx);
        }
        scPutU16le(crcBytes, storedCrc(p));
        backend_.update((uint16_t)(addr + size), crcBytes, EepromLayout::CrcSize, backend_.ctx);
    }

    /* 2) Huvudet sist: avbrott under 1) ger ogiltig CRC/huvud -> standardvarden. */
    uint8_t h[EepromLayout::HeaderSize];
    h[0] = EepromLayout::Magic0;
    h[1] = EepromLayout::Magic1;
    h[2] = EepromLayout::LayoutVersion;
    h[3] = EepromLayout::PageCount;
    scPutU16le(&h[EepromLayout::HeaderDataSize], crc16(h, EepromLayout::HeaderDataSize));
    backend_.update(EepromLayout::BaseAddress, h, EepromLayout::HeaderSize, backend_.ctx);

    /* 3) Verifiera genom aterlasning (bara CRC, ingen RAM-buffert behovs). */
    bool ok = headerValid();
    for (uint8_t p = 0U; ok && (p < EepromLayout::PageCount); ++p)
    {
        const uint16_t size = EepromLayout::pageSize(p);
        const uint16_t addr = EepromLayout::pageAddress(p);
        uint16_t crc = Crc16Init;
        for (uint16_t i = 0U; i < size; ++i)
        {
            uint8_t b;
            backend_.read((uint16_t)(addr + i), &b, 1U, backend_.ctx);
            crc = crc16Update(crc, b);
        }
        uint8_t crcBytes[EepromLayout::CrcSize];
        backend_.read((uint16_t)(addr + size), crcBytes, EepromLayout::CrcSize, backend_.ctx);
        ok = (crc == scGetU16le(crcBytes)) && (crc == storedCrc(p));
    }

    return ok;
}

uint8_t* EepromStore::page(uint8_t index)
{
    return (index < EepromLayout::PageCount) ? &data_[EepromLayout::pageRamOffset(index)] : 0;
}

const uint8_t* EepromStore::page(uint8_t index) const
{
    return (index < EepromLayout::PageCount) ? &data_[EepromLayout::pageRamOffset(index)] : 0;
}

uint16_t EepromStore::pageSize(uint8_t index) const
{
    return EepromLayout::pageSize(index);
}

bool EepromStore::rangeOk(uint8_t index, uint16_t offset, uint16_t len) const
{
    const uint16_t size = EepromLayout::pageSize(index);
    /* Skrivet utan addition sa att offset + len inte kan sla runt. */
    return (index < EepromLayout::PageCount) && (offset <= size) && (len <= (uint16_t)(size - offset));
}

bool EepromStore::readBytes(uint8_t index, uint16_t offset, uint8_t* dst, uint16_t len) const
{
    if (!rangeOk(index, offset, len) || ((dst == 0) && (len != 0U)))
    {
        return false;
    }
    if (len != 0U)
    {
        memcpy(dst, &data_[EepromLayout::pageRamOffset(index) + offset], len);
    }
    return true;
}

bool EepromStore::writeBytes(uint8_t index, uint16_t offset, const uint8_t* src, uint16_t len)
{
    if (!rangeOk(index, offset, len) || ((src == 0) && (len != 0U)))
    {
        return false;
    }
    if (len != 0U)
    {
        memcpy(&data_[EepromLayout::pageRamOffset(index) + offset], src, len);
    }
    return true;
}
