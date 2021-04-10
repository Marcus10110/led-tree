#include <emscripten/bind.h>
#include <emscripten/emscripten.h>

#include "animations.h"
#include <array>
#include <cstdint>
#include <functional>
#include <vector>


using namespace emscripten;

struct RgbColor
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

EMSCRIPTEN_KEEPALIVE int Sum( int a, int b )
{
    return a + b;
}

std::vector<RgbColor> Render( int animation_id, int strip_id, int time_ms, int led_count );

EMSCRIPTEN_BINDINGS( led_module )
{
    function( "AnimationCount", &Animation::AnimationCount );
    function( "Sum", &Sum );
    function( "Render", &Render );

    value_object<RgbColor>( "RgbColor" ).field( "r", &RgbColor::r ).field( "g", &RgbColor::g ).field( "b", &RgbColor::b );

    register_vector<RgbColor>( "vector<RgbColor>" );
}


struct StripHistory
{
    std::vector<RgbColor> mLeds;
    int mLastAnimation{ 0 };
};

std::map<int, StripHistory> mStripHistories;

EMSCRIPTEN_KEEPALIVE std::vector<RgbColor> Render( int animation_id, int strip_id, int time_ms, int led_count )
{
    auto& history = mStripHistories[ strip_id ];
    auto& leds = history.mLeds;
    if( history.mLastAnimation != animation_id )
    {
        leds.clear();
    }
    history.mLastAnimation = animation_id;
    if( leds.size() < led_count )
    {
        leds.resize( led_count, { 0, 0, 0 } );
    }

    auto set_led = [&leds]( int index, uint8_t r, uint8_t g, uint8_t b ) { leds.at( index ) = { r, g, b }; };
    auto get_led = [&leds]( int index, uint8_t& r, uint8_t& g, uint8_t& b ) {
        auto led = leds.at( index );
        r = led.r;
        g = led.g;
        b = led.b;
    };

    Animation::AnimationSettings settings{ led_count, strip_id };

    auto& animation = *Animation::GetAnimation( animation_id );

    animation.Reset( settings );
    animation.Render( set_led, get_led, static_cast<uint32_t>( time_ms ) );

    return leds;
}