//--------------------------------------------------------------------------------------
// File: SpriteSheet.h
//
// C++ sprite sheet renderer
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------

#pragma once

#include <exception>
#include <fstream>
#include <locale>
#include <map>
#include <stdexcept>
#include <string>

#include "SpriteBatch.h"

#include <wrl/client.h>

#ifdef __clang__
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#pragma clang diagnostic ignored "-Wswitch-enum"
#endif

#pragma warning(disable : 4061)

class SpriteSheet
{
public:
    SpriteSheet() = default;
    ~SpriteSheet() = default;

    SpriteSheet(SpriteSheet&&) = default;
    SpriteSheet& operator= (SpriteSheet&&) = default;

    SpriteSheet(SpriteSheet const&) = delete;
    SpriteSheet& operator= (SpriteSheet const&) = delete;

    struct SpriteFrame
    {
        RECT                sourceRect;
        DirectX::XMFLOAT2   size;
        DirectX::XMFLOAT2   origin;
        bool                rotated;
    };

    void Load(ID3D11ShaderResourceView* texture, const wchar_t* szFileName)
    {
        mSprites.clear();

        mTexture = texture;

        if (szFileName)
        {
            //
            // This code parses the 'MonoGame' project txt file that is produced by CodeAndWeb's TexturePacker.
            // https://www.codeandweb.com/texturepacker
            //
            // You can modify it to match whatever sprite-sheet tool you are using
            //

            std::wifstream inFile(szFileName);
            if (!inFile)
                throw std::runtime_error("SpriteSheet failed to load .txt data");

            inFile.imbue(std::locale::classic());

            wchar_t strLine[1024] = {};
            for (;;)
            {
                inFile >> strLine;
                if (!inFile)
                    break;

                if (0 == wcscmp(strLine, L"#"))
                {
                    // Comment
                }
                else
                {
                    // Parse lines of form: Name;rotatedInt;xInt;yInt;widthInt;heightInt;origWidthInt;origHeightInt;offsetXFloat;offsetYFloat
                    static const wchar_t* delim = L";\n";

                    wchar_t* context = nullptr;
                    wchar_t* name = wcstok_s(strLine, delim, &context);
                    if (!name || !*name)
                        throw std::runtime_error("SpriteSheet encountered invalid .txt data");

                    if (mSprites.find(name) != mSprites.cend())
                        throw std::runtime_error("SpriteSheet encountered duplicate in .txt data");

                    wchar_t* str = wcstok_s(nullptr, delim, &context);
                    if (!str)
                        throw std::runtime_error("SpriteSheet encountered invalid .txt data");

                    SpriteFrame frame;
                    frame.rotated = (_wtoi(str) == 1);

                    str = wcstok_s(nullptr, delim, &context);
                    if (!str)
                        throw std::runtime_error("SpriteSheet encountered invalid .txt data");
                    frame.sourceRect.left = _wtol(str);

                    str = wcstok_s(nullptr, delim, &context);
                    if (!str)
                        throw std::runtime_error("SpriteSheet encountered invalid .txt data");
                    frame.sourceRect.top = _wtol(str);

                    str = wcstok_s(nullptr, delim, &context);
                    if (!str)
                        throw std::runtime_error("SpriteSheet encountered invalid .txt data");
                    LONG dx = _wtol(str);
                    frame.sourceRect.right = frame.sourceRect.left + dx;

                    str = wcstok_s(nullptr, delim, &context);
                    if (!str)
                        throw std::runtime_error("SpriteSheet encountered invalid .txt data");
                    LONG dy = +_wtol(str);
                    frame.sourceRect.bottom = frame.sourceRect.top + dy;

                    str = wcstok_s(nullptr, delim, &context);
                    if (!str)
                        throw std::runtime_error("SpriteSheet encountered invalid .txt data");
                    frame.size.x = static_cast<float>(_wtof(str));

                    str = wcstok_s(nullptr, delim, &context);
                    if (!str)
                        throw std::runtime_error("SpriteSheet encountered invalid .txt data");
                    frame.size.y = static_cast<float>(_wtof(str));

                    str = wcstok_s(nullptr, delim, &context);
                    if (!str)
                        throw std::runtime_error("SpriteSheet encountered invalid .txt data");
                    float pivotX = static_cast<float>(_wtof(str));

                    str = wcstok_s(nullptr, delim, &context);
                    if (!str)
                        throw std::runtime_error("SpriteSheet encountered invalid .txt data");
                    float pivotY = static_cast<float>(_wtof(str));

                    if (frame.rotated)
                    {
                        frame.origin.x = float(dx) * (1.f - pivotY);
                        frame.origin.y = float(dy) * pivotX;
                    }
                    else
                    {
                        frame.origin.x = float(dx) * pivotX;
                        frame.origin.y = float(dy) * pivotY;
                    }

                    mSprites.insert(std::pair<std::wstring, SpriteFrame>(std::wstring(name), frame));
                }

                inFile.ignore(1000, '\n');
            }
        }
    }

    const SpriteFrame* Find(const wchar_t* name) const
    {
        auto it = mSprites.find(name);
        if (it == mSprites.cend())
            return nullptr;

        return &it->second;
    }

    // Draw overloads specifying position and scale as XMFLOAT2.
    void XM_CALLCONV Draw(DirectX::SpriteBatch* batch, const SpriteFrame& frame, DirectX::XMFLOAT2 const& position,
        DirectX::FXMVECTOR color = DirectX::Colors::White, float rotation = 0, float scale = 1,
        DirectX::SpriteEffects effects = DirectX::SpriteEffects_None, float layerDepth = 0) const
    {
        assert(batch != nullptr);
        using namespace DirectX;

        if (frame.rotated)
        {
            rotation -= XM_PIDIV2;
            switch (effects)
            {
            case SpriteEffects_FlipHorizontally:    effects = SpriteEffects_FlipVertically; break;
            case SpriteEffects_FlipVertically:      effects = SpriteEffects_FlipHorizontally; break;
            default: break;
            }
        }

        XMFLOAT2 origin = frame.origin;
        switch (effects)
        {
        case SpriteEffects_FlipHorizontally:    origin.x = float(frame.sourceRect.right - frame.sourceRect.left) - origin.x; break;
        case SpriteEffects_FlipVertically:      origin.y = float(frame.sourceRect.bottom - frame.sourceRect.top) - origin.y; break;
        default: break;
        }

        batch->Draw(mTexture.Get(), position, &frame.sourceRect, color, rotation, origin, scale, effects, layerDepth);
    }

    void XM_CALLCONV Draw(DirectX::SpriteBatch* batch, const SpriteFrame& frame, DirectX::XMFLOAT2 const& position,
        DirectX::FXMVECTOR color, float rotation, DirectX::XMFLOAT2 const& scale,
        DirectX::SpriteEffects effects = DirectX::SpriteEffects_None, float layerDepth = 0) const

    {
        assert(batch != nullptr);
        using namespace DirectX;

        if (frame.rotated)
        {
            rotation -= XM_PIDIV2;
            switch (effects)
            {
            case SpriteEffects_FlipHorizontally:    effects = SpriteEffects_FlipVertically; break;
            case SpriteEffects_FlipVertically:      effects = SpriteEffects_FlipHorizontally; break;
            default: break;
            }
        }

        XMFLOAT2 origin = frame.origin;
        switch (effects)
        {
        case SpriteEffects_FlipHorizontally:    origin.x = float(frame.sourceRect.right - frame.sourceRect.left) - origin.x; break;
        case SpriteEffects_FlipVertically:      origin.y = float(frame.sourceRect.bottom - frame.sourceRect.top) - origin.y; break;
        default: break;
        }

        batch->Draw(mTexture.Get(), position, &frame.sourceRect, color, rotation, origin, scale, effects, layerDepth);
    }

    // Draw overloads specifying position and scale via the first two components of an XMVECTOR.
    void XM_CALLCONV Draw(DirectX::SpriteBatch* batch, const SpriteFrame& frame, DirectX::FXMVECTOR position,
        DirectX::FXMVECTOR color = DirectX::Colors::White, float rotation = 0, float scale = 1,
        DirectX::SpriteEffects effects = DirectX::SpriteEffects_None, float layerDepth = 0) const
    {
        assert(batch != nullptr);
        using namespace DirectX;

        if (frame.rotated)
        {
            rotation -= XM_PIDIV2;
            switch (effects)
            {
            case SpriteEffects_FlipHorizontally:    effects = SpriteEffects_FlipVertically; break;
            case SpriteEffects_FlipVertically:      effects = SpriteEffects_FlipHorizontally; break;
            default: break;
            }
        }

        XMFLOAT2 origin = frame.origin;
        switch (effects)
        {
        case SpriteEffects_FlipHorizontally:    origin.x = float(frame.sourceRect.right - frame.sourceRect.left) - origin.x; break;
        case SpriteEffects_FlipVertically:      origin.y = float(frame.sourceRect.bottom - frame.sourceRect.top) - origin.y; break;
        default: break;
        }
        XMVECTOR vorigin = XMLoadFloat2(&origin);

        batch->Draw(mTexture.Get(), position, &frame.sourceRect, color, rotation, vorigin, scale, effects, layerDepth);
    }

    void XM_CALLCONV Draw(DirectX::SpriteBatch* batch, const SpriteFrame& frame, DirectX::FXMVECTOR position,
        DirectX::FXMVECTOR color, float rotation, DirectX::GXMVECTOR scale,
        DirectX::SpriteEffects effects = DirectX::SpriteEffects_None, float layerDepth = 0) const
    {
        assert(batch != nullptr);
        using namespace DirectX;

        if (frame.rotated)
        {
            rotation -= XM_PIDIV2;
            switch (effects)
            {
            case SpriteEffects_FlipHorizontally:    effects = SpriteEffects_FlipVertically; break;
            case SpriteEffects_FlipVertically:      effects = SpriteEffects_FlipHorizontally; break;
            default: break;
            }
        }

        XMFLOAT2 origin = frame.origin;
        switch (effects)
        {
        case SpriteEffects_FlipHorizontally:    origin.x = float(frame.sourceRect.right - frame.sourceRect.left) - origin.x; break;
        case SpriteEffects_FlipVertically:      origin.y = float(frame.sourceRect.bottom - frame.sourceRect.top) - origin.y; break;
        default: break;
        }
        XMVECTOR vorigin = XMLoadFloat2(&origin);

        batch->Draw(mTexture.Get(), position, &frame.sourceRect, color, rotation, vorigin, scale, effects, layerDepth);
    }

    // Draw overloads specifying position as a RECT.
    void XM_CALLCONV Draw(DirectX::SpriteBatch* batch, const SpriteFrame& frame, RECT const& destinationRectangle,
        DirectX::FXMVECTOR color = DirectX::Colors::White, float rotation = 0,
        DirectX::SpriteEffects effects = DirectX::SpriteEffects_None, float layerDepth = 0) const
    {
        assert(batch != nullptr);
        using namespace DirectX;

        if (frame.rotated)
        {
            rotation -= XM_PIDIV2;
            switch (effects)
            {
            case SpriteEffects_FlipHorizontally:    effects = SpriteEffects_FlipVertically; break;
            case SpriteEffects_FlipVertically:      effects = SpriteEffects_FlipHorizontally; break;
            default: break;
            }
        }

        XMFLOAT2 origin = frame.origin;
        switch (effects)
        {
        case SpriteEffects_FlipHorizontally:    origin.x = float(frame.sourceRect.right - frame.sourceRect.left) - origin.x; break;
        case SpriteEffects_FlipVertically:      origin.y = float(frame.sourceRect.bottom - frame.sourceRect.top) - origin.y; break;
        default: break;
        }

        batch->Draw(mTexture.Get(), destinationRectangle, &frame.sourceRect, color, rotation, origin, effects, layerDepth);
    }

private:
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    mTexture;
    std::map<std::wstring, SpriteFrame>                 mSprites;
};
