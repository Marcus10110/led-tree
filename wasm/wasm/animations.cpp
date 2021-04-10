#include "animations.h"
#include <array>
#include <stdlib.h>
#include <limits>
namespace Animation
{
    Animation::Animation() = default;
    Animation::~Animation() = default;

    void Animation::Reset( const AnimationSettings& settings )
    {
        mSettings = settings;
    }

    namespace
    {
        int random( int max )
        {
            return rand() % max;
        }

        int random( int min, int max )
        {
            return ( rand() % ( max - min ) ) + min;
        }

        std::tuple<uint8_t, uint8_t, uint8_t> HsvToRgb( uint16_t h16, uint8_t s8, uint8_t v8 )
        {
            const float h = h16 / 65535.;
            const float s = s8 / 255.;
            const float v = v8 / 255.;

            float r = 0, g = 0, b = 0;


            float hh, p, q, t, ff;
            long i;

            hh = h;
            if( hh >= 360.0 )
                hh = 0.0;
            hh /= 60.0;
            i = ( long )hh;
            ff = hh - i;
            p = v * ( 1.0 - s );
            q = v * ( 1.0 - ( s * ff ) );
            t = v * ( 1.0 - ( s * ( 1.0 - ff ) ) );

            switch( i )
            {
            case 0:
                r = v;
                g = t;
                b = p;
                break;
            case 1:
                r = q;
                g = v;
                b = p;
                break;
            case 2:
                r = p;
                g = v;
                b = t;
                break;

            case 3:
                r = p;
                g = q;
                b = v;
                break;
            case 4:
                r = t;
                g = p;
                b = v;
                break;
            case 5:
            default:
                r = v;
                g = p;
                b = q;
                break;
            }
            return std::make_tuple( static_cast<uint8_t>( r * 255 ), static_cast<uint8_t>( g * 255 ), static_cast<uint8_t>( b * 255 ) );
        }

        std::tuple<uint16_t, uint8_t, uint8_t> RgbToHsv( uint8_t r8, uint8_t g8, uint8_t b8 )
        {
            const float r = r8 / 255.;
            const float g = g8 / 255.;
            const float b = b8 / 255.;

            float h = 0;
            float s = 0;
            float v = 0;

            double min, max, delta;

            min = r < g ? r : g;
            min = min < b ? min : b;

            max = r > g ? r : g;
            max = max > b ? max : b;

            v = max; // v
            delta = max - min;
            if( delta < 0.00001 )
            {
                return {};
            }
            if( max > 0.0 )
            {                        // NOTE: if Max is == 0, this divide would cause a crash
                s = ( delta / max ); // s
            }
            else
            {
                return {};
            }
            if( r >= max )             // > is bogus, just keeps compilor happy
                h = ( g - b ) / delta; // between yellow & magenta
            else if( g >= max )
                h = 2.0 + ( b - r ) / delta; // between cyan & yellow
            else
                h = 4.0 + ( r - g ) / delta; // between magenta & cyan

            h *= 60.0; // degrees

            if( h < 0.0 )
                h += 360.0;

            return std::make_tuple( static_cast<uint16_t>( h / 360.0 * 65535. ), static_cast<uint8_t>( s * 255 ),
                                    static_cast<uint8_t>( v * 255 ) );
        }
        class FnAnimation : public Animation
        {
          public:
            using AnimationFn = std::function<void( SetLedFn&, GetLedFn&, const AnimationSettings&, uint32_t )>;
            FnAnimation( AnimationFn fn ) : mFn( std::move( fn ) )
            {
            }
            ~FnAnimation() override = default;
            void Reset( const AnimationSettings& settings ) override
            {
                Animation::Reset( settings );
            }
            void Render( SetLedFn set_led, GetLedFn get_led, uint32_t ms ) override
            {
                mFn( set_led, get_led, mSettings, ms );
            }

          protected:
            AnimationFn mFn;
        };

        FnAnimation WhiteAnimation =
            FnAnimation( []( SetLedFn& set_led, GetLedFn& get_led, const AnimationSettings& settings, uint32_t ms ) {
                for( int i = 0; i < settings.mCount; ++i )
                {
                    set_led( i, 255, 255, 255 );
                }
            } );

        FnAnimation StripIdAnimation =
            FnAnimation( []( SetLedFn& set_led, GetLedFn& get_led, const AnimationSettings& settings, uint32_t ms ) {
                for( int i = 0; i < settings.mCount; ++i )
                {
                    if( i < settings.mIndex * 10 + 5 )
                    {
                        set_led( i, 255, 255, 255 );
                    }
                    else
                    {
                        set_led( i, 0, 0, 0 );
                    }
                }
            } );


        FnAnimation CircleRainbowAnimation =
            FnAnimation( []( SetLedFn& set_led, GetLedFn& get_led, const AnimationSettings& settings, uint32_t ms ) {
                float start = ( ( ( ms % 5000 ) / 5000. ) + ( settings.mIndex / 4.0 ) ) * 65535.0;
                const float step = ( ( 256.0 / settings.mCount ) / 4.0 );
                for( int i = 0; i < settings.mCount; ++i )
                {
                    start += step;
                    while( start > 255 )
                    {
                        start = start - 255;
                    }
                    auto rgb = HsvToRgb( static_cast<uint16_t>( start ), 255, 255 );
                    set_led( i, std::get<0>( rgb ), std::get<1>( rgb ), std::get<2>( rgb ) );
                }
            } );


        FnAnimation SparkleAnimation =
            FnAnimation( []( SetLedFn& set_led, GetLedFn& get_led, const AnimationSettings& settings, uint32_t ms ) {
                // TODO: lets get way more sparkles, but make more of them dim.

                // each pixel has a random change of sparkling.
                // lets say that we want about one third of the strip to sparkle each
                // second on average.
                // every second, we want N new stars.
                // odds = LED_COUNT * fps / stars per sec
                constexpr int odds = ( 30 * 600 ) / ( 180 );

                for( int i = 0; i < settings.mCount; i++ )
                {
                    if( random( odds ) == 0 )
                    {
                        // sparkle!
                        float hue = random( 100 ) / 100.0;            // random color
                        float brightness = random( 20, 100 ) / 100.0; // all are at least 50% brightness.
                        brightness = brightness * brightness * brightness * brightness * brightness * brightness;
                        float saturation = random( 10, 100 ) / 100.0; // some color?
                        auto color = HsvToRgb( static_cast<uint16_t>( hue * std::numeric_limits<uint16_t>::max() ),
                                               static_cast<uint8_t>( saturation * 255 ), static_cast<uint8_t>( brightness * 255 ) );
                        set_led( i, std::get<0>( color ), std::get<1>( color ), std::get<2>( color ) );
                        if( brightness > 0.65 )
                        {
                            // lets glow the neighbors.
                            color = HsvToRgb( static_cast<uint16_t>( hue * std::numeric_limits<uint16_t>::max() ),
                                              static_cast<uint8_t>( saturation * 255 ), static_cast<uint8_t>( brightness * 255 / 4 ) );
                            if( i >= 1 )
                            {
                                set_led( i - 1, std::get<0>( color ), std::get<1>( color ), std::get<2>( color ) );
                            }
                            if( i < ( settings.mCount - 1 ) )
                            {
                                set_led( i + 1, std::get<0>( color ), std::get<1>( color ), std::get<2>( color ) );
                            }
                        }
                    }
                    else
                    {
                        // decay.
                        uint8_t r, g, b;
                        get_led( i, r, g, b );
                        auto hsv = RgbToHsv( r, g, b );
                        if( std::get<2>( hsv ) > 0.0 )
                        {
                            // decay over 5 seconds, from 255 to 0, at 30 hz. 30*5 =150 frames,
                            std::get<2>( hsv ) = std::max( 0, std::get<2>( hsv ) - 3 );
                        }
                        auto rgb = HsvToRgb( std::get<0>( hsv ), std::get<1>( hsv ), std::get<2>( hsv ) );
                        set_led( i, std::get<0>( rgb ), std::get<1>( rgb ), std::get<2>( rgb ) );
                    }
                }
            } );

        std::array<Animation*, 4> AllAnimations = { &WhiteAnimation, &StripIdAnimation, &CircleRainbowAnimation, &SparkleAnimation };
    }

    int AnimationCount()
    {
        return AllAnimations.size();
    }
    Animation* GetAnimation( int animation_index )
    {
        return AllAnimations.at( animation_index );
    }
}