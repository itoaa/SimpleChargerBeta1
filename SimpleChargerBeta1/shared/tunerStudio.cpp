/*
 * tunerStudio.cpp - protokoll v2.2, se tunerStudio.hpp.
 */
#include "tunerStudio.hpp"
#include <string.h>

/* TunerStudio jamfor dessa byte. Avslutande NUL skickas inte. */
static const char codeString[] = "MSII s-Charger 2.2  ";
static const char codeRev[] = "Smedby simple Charger Beta 1 v2.2";
static const char protocolVersion[] = "002";

typedef char tsSigLen[(sizeof(codeString) - 1U == 20U) ? 1 : -1];

TunerStudio::TunerStudio(EepromStore& store)
    : store_(store), state_(Idle), page_(0U), offset_(0U), count_(0U), got_(0U),
      lastByteMs_(0U), sendFn_(0), sendCtx_(0)
{
    hooks_.burn = 0;
    hooks_.command = 0;
    hooks_.pageWritten = 0;
    hooks_.ctx = 0;
    memset(realtime, 0, sizeof(realtime));
    memset(chunk_, 0, sizeof(chunk_));
}

void TunerStudio::setSend(void (*send)(const uint8_t* data, uint8_t len, void* ctx), void* ctx)
{
    sendFn_ = send;
    sendCtx_ = ctx;
}

void TunerStudio::setHooks(const Hooks& hooks)
{
    hooks_ = hooks;
}

void TunerStudio::send(const uint8_t* data, uint8_t len)
{
    if (sendFn_ != 0)
    {
        sendFn_(data, len, sendCtx_);
    }
}

void TunerStudio::applyWrite(const uint8_t* src, uint8_t len)
{
    /* writeBytes gransskontrollerar och andrar inget vid fel. */
    if (store_.writeBytes(page_, offset_, src, len))
    {
        if (hooks_.pageWritten != 0)
        {
            hooks_.pageWritten(page_, hooks_.ctx);
        }
    }
}

void TunerStudio::handleCommand(uint8_t command)
{
    uint8_t one;
    switch (command)
    {
    case 'A':
        send(realtime, (uint8_t)TsLayout::RtBlockSize);
        break;
    case 'c':
    case 'C':
        one = realtime[TsLayout::RtSeconds];
        send(&one, 1U);
        break;
    case 'S':
        send(reinterpret_cast<const uint8_t*>(codeRev), (uint8_t)(sizeof(codeRev) - 1U));
        break;
    case 'Q':
        send(reinterpret_cast<const uint8_t*>(codeString), (uint8_t)(sizeof(codeString) - 1U));
        break;
    case 'F':
        send(reinterpret_cast<const uint8_t*>(protocolVersion), (uint8_t)(sizeof(protocolVersion) - 1U));
        break;
    case 'V':
        send(store_.page(page_), (uint8_t)store_.pageSize(page_));
        break;
    case 'P':
        state_ = PPage;
        break;
    case 'E':
        state_ = EData;
        break;
    case 'W':
        state_ = WOffset;
        break;
    case 'X':
        state_ = XOffset;
        break;
    case 'B':
        if (hooks_.burn != 0)
        {
            hooks_.burn(hooks_.ctx);
        }
        break;
    default:
        break;
    }
}

void TunerStudio::poll(uint32_t nowMs)
{
    if ((state_ != Idle) && ((uint32_t)(nowMs - lastByteMs_) > (uint32_t)TimeoutMs))
    {
        state_ = Idle;
    }
}

void TunerStudio::onByte(uint8_t byte, uint32_t nowMs)
{
    poll(nowMs);
    lastByteMs_ = nowMs;
    switch (state_)
    {
    case PPage:
        if (byte < EepromLayout::PageCount)
        {
            page_ = byte;
        }
        state_ = Idle;
        break;
    case EData:
        state_ = Idle;
        if (hooks_.command != 0)
        {
            hooks_.command(byte, hooks_.ctx);
        }
        break;
    case WOffset:
        offset_ = byte;
        state_ = WValue;
        break;
    case WValue:
        state_ = Idle;
        applyWrite(&byte, 1U);
        break;
    case XOffset:
        offset_ = byte;
        state_ = XCount;
        break;
    case XCount:
        count_ = byte;
        got_ = 0U;
        if (count_ == 0U)
        {
            state_ = Idle;
        }
        else if ((count_ > (uint8_t)MaxChunk) ||
                 ((uint16_t)((uint16_t)offset_ + count_) > store_.pageSize(page_)))
        {
            /* Hela blocket avvisas, men datat maste lasas bort. */
            state_ = XDiscard;
        }
        else
        {
            state_ = XData;
        }
        break;
    case XData:
        chunk_[got_] = byte;
        got_++;
        if (got_ == count_)
        {
            state_ = Idle;
            applyWrite(chunk_, count_);
        }
        break;
    case XDiscard:
        got_++;
        if (got_ == count_)
        {
            state_ = Idle;
        }
        break;
    case Idle:
    default:
        handleCommand(byte);
        break;
    }
}
