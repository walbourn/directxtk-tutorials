//--------------------------------------------------------------------------------------
// File: ScrollingBackground.h
//
// C++ version of the C# example on how to make a scrolling background with SpriteBatch
// http://msdn.microsoft.com/en-us/library/bb203868.aspx
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------

#pragma once

#include <exception>
#include <stdexcept>

#include "SpriteBatch.h"

#include <wrl/client.h>

class ScrollingBackground
{
public:
    ScrollingBackground() noexcept :
        mScreenHeight(0),
        mTextureWidth(0),
        mTextureHeight(0),
        mScreenPos{},
        mTextureSize{},
        mOrigin{}
    {
    }

    ScrollingBackground(ScrollingBackground&&) = default;
    ScrollingBackground& operator= (ScrollingBackground&&) = default;

    ScrollingBackground(ScrollingBackground const&) = default;
    ScrollingBackground& operator= (ScrollingBackground const&) = default;

    void Load(ID3D11ShaderResourceView* texture)
    {
        mTexture = texture;

        if (texture)
        {
            Microsoft::WRL::ComPtr<ID3D11Resource> resource;
            texture->GetResource(resource.GetAddressOf());

            D3D11_RESOURCE_DIMENSION dim;
            resource->GetType(&dim);

            if (dim != D3D11_RESOURCE_DIMENSION_TEXTURE2D)
                throw std::runtime_error("ScrollingBackground expects a Texture2D");

            Microsoft::WRL::ComPtr<ID3D11Texture2D> tex2D;
            resource.As(&tex2D);

            D3D11_TEXTURE2D_DESC desc;
            tex2D->GetDesc(&desc);

            mTextureWidth = int(desc.Width);
            mTextureHeight = int(desc.Height);

            mTextureSize.x = 0.f;
            mTextureSize.y = float(desc.Height);

            mOrigin.x = float(desc.Width) / 2.f;
            mOrigin.y = 0.f;
        }
    }

    void SetWindow(int screenWidth, int screenHeight)
    {
        mScreenHeight = screenHeight;

        mScreenPos.x = float(screenWidth) / 2.f;
        mScreenPos.y = float(screenHeight) / 2.f;
    }

    void Update(float deltaY)
    {
        mScreenPos.y += deltaY;
        mScreenPos.y = fmodf(mScreenPos.y, float(mTextureHeight));
    }

    void Draw(DirectX::SpriteBatch* batch) const
    {
        using namespace DirectX;

        XMVECTOR screenPos = XMLoadFloat2(&mScreenPos);
        XMVECTOR origin = XMLoadFloat2(&mOrigin);

        if (mScreenPos.y < float(mScreenHeight))
        {
            batch->Draw(mTexture.Get(), screenPos, nullptr,
                Colors::White, 0.f, origin, g_XMOne, SpriteEffects_None, 0.f);
        }

        XMVECTOR textureSize = XMLoadFloat2(&mTextureSize);

        screenPos -= textureSize;

        batch->Draw(mTexture.Get(), screenPos, nullptr,
            Colors::White, 0.f, origin, g_XMOne, SpriteEffects_None, 0.f);

        int pixels = static_cast<int>(XMVectorGetY(screenPos)) + mTextureHeight;
        while (pixels < mScreenHeight)
        {
            screenPos += textureSize;
            pixels += mTextureHeight;

            batch->Draw(mTexture.Get(), screenPos, nullptr,
                Colors::White, 0.f, origin, g_XMOne, SpriteEffects_None, 0.f);
        }
    }

private:
    int                                                 mScreenHeight;
    int                                                 mTextureWidth;
    int                                                 mTextureHeight;
    DirectX::XMFLOAT2                                   mScreenPos;
    DirectX::XMFLOAT2                                   mTextureSize;
    DirectX::XMFLOAT2                                   mOrigin;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    mTexture;
};
