//--------------------------------------------------------------------------------------
// File: AnimatedTexture.h
//
// C++ version of the C# example on how to animate a 2D sprite using SpriteBatch
// http://msdn.microsoft.com/en-us/library/bb203866.aspx
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------

#pragma once

#include <stdexcept>
#include <SpriteBatch.h>

#include <wrl/client.h>

class AnimatedTexture
{
public:
    AnimatedTexture() noexcept :
        mPaused(false),
        mFrame(0),
        mFrameCount(0),
        mTextureWidth(0),
        mTextureHeight(0),
        mTimePerFrame(0.f),
        mTotalElapsed(0.f),
        mDepth(0.f),
        mRotation(0.f),
        mOrigin{},
        mScale(1.f, 1.f)
    {
    }

    AnimatedTexture(const DirectX::XMFLOAT2& origin,
        float rotation,
        float scale,
        float depth) noexcept :
        mPaused(false),
        mFrame(0),
        mFrameCount(0),
        mTextureWidth(0),
        mTextureHeight(0),
        mTimePerFrame(0.f),
        mTotalElapsed(0.f),
        mDepth(depth),
        mRotation(rotation),
        mOrigin(origin),
        mScale(scale, scale)
    {
    }

    AnimatedTexture(AnimatedTexture&&) = default;
    AnimatedTexture& operator= (AnimatedTexture&&) = default;

    AnimatedTexture(AnimatedTexture const&) = default;
    AnimatedTexture& operator= (AnimatedTexture const&) = default;

    void Load(ID3D11ShaderResourceView* texture, int frameCount, int framesPerSecond)
    {
        if (frameCount < 0 || framesPerSecond <= 0)
            throw std::invalid_argument("AnimatedTexture");

        mPaused = false;
        mFrame = 0;
        mFrameCount = frameCount;
        mTimePerFrame = 1.f / float(framesPerSecond);
        mTotalElapsed = 0.f;
        mTexture = texture;

        if (texture)
        {
            Microsoft::WRL::ComPtr<ID3D11Resource> resource;
            texture->GetResource(resource.GetAddressOf());

            D3D11_RESOURCE_DIMENSION dim;
            resource->GetType(&dim);

            if (dim != D3D11_RESOURCE_DIMENSION_TEXTURE2D)
                throw std::runtime_error("AnimatedTexture expects a Texture2D");

            Microsoft::WRL::ComPtr<ID3D11Texture2D> tex2D;
            resource.As(&tex2D);

            D3D11_TEXTURE2D_DESC desc;
            tex2D->GetDesc(&desc);

            mTextureWidth = int(desc.Width);
            mTextureHeight = int(desc.Height);
        }
    }

    void Update(float elapsed)
    {
        if (mPaused)
            return;

        mTotalElapsed += elapsed;

        if (mTotalElapsed > mTimePerFrame)
        {
            ++mFrame;
            mFrame = mFrame % mFrameCount;
            mTotalElapsed -= mTimePerFrame;
        }
    }

    void Draw(DirectX::SpriteBatch* batch, const DirectX::XMFLOAT2& screenPos) const
    {
        Draw(batch, mFrame, screenPos);
    }

    void Draw(DirectX::SpriteBatch* batch, int frame, const DirectX::XMFLOAT2& screenPos) const
    {
        int frameWidth = mTextureWidth / mFrameCount;

        RECT sourceRect;
        sourceRect.left = frameWidth * frame;
        sourceRect.top = 0;
        sourceRect.right = sourceRect.left + frameWidth;
        sourceRect.bottom = mTextureHeight;

        batch->Draw(mTexture.Get(), screenPos, &sourceRect, DirectX::Colors::White,
            mRotation, mOrigin, mScale, DirectX::SpriteEffects_None, mDepth);
    }

    void Reset()
    {
        mFrame = 0;
        mTotalElapsed = 0.f;
    }

    void Stop()
    {
        mPaused = true;
        mFrame = 0;
        mTotalElapsed = 0.f;
    }

    void Play() { mPaused = false; }
    void Paused() { mPaused = true; }

    bool IsPaused() const { return mPaused; }

private:
    bool                                                mPaused;
    int                                                 mFrame;
    int                                                 mFrameCount;
    int                                                 mTextureWidth;
    int                                                 mTextureHeight;
    float                                               mTimePerFrame;
    float                                               mTotalElapsed;
    float                                               mDepth;
    float                                               mRotation;
    DirectX::XMFLOAT2                                   mOrigin;
    DirectX::XMFLOAT2                                   mScale;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    mTexture;
};
