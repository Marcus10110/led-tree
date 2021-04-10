#pragma once
#include <cstdint>
#include <functional>
namespace Animation
{
    using SetLedFn = std::function<void( int, uint8_t, uint8_t, uint8_t )>;
    using GetLedFn = std::function<void( int, uint8_t&, uint8_t&, uint8_t& )>;

    struct AnimationSettings
    {
        int mCount;
        int mIndex;
    };

    class Animation
    {
      public:
        Animation();
        virtual ~Animation();
        virtual void Reset( const AnimationSettings& settings );
        virtual void Render( SetLedFn set_led, GetLedFn get_led, uint32_t ms ) = 0;

      protected:
        AnimationSettings mSettings;
    };

    int AnimationCount();
    Animation* GetAnimation( int animation_index );
}