#include <emscripten/bind.h>
#include <emscripten/emscripten.h>

#include "animations.h"
#include <array>
#include <cstdint>
#include <functional>
#include <vector>


using namespace emscripten;

struct RgbColor {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

struct HsvColor {
  uint8_t h;
  uint8_t s;
  uint8_t v;
};

struct HsvColorF {
  double h;
  double s;
  double v;
};

struct RgbColorF {
  double r;
  double g;
  double b;
};

RgbColor hsv2rgb(HsvColor in8) {

  HsvColorF in = HsvColorF{in8.h / 255.0 * 360.0, in8.s / 255.0, in8.v / 255.0};

  double hh, p, q, t, ff;
  long i;
  RgbColorF out;

  hh = in.h;
  if (hh >= 360.0)
    hh = 0.0;
  hh /= 60.0;
  i = (long)hh;
  ff = hh - i;
  p = in.v * (1.0 - in.s);
  q = in.v * (1.0 - (in.s * ff));
  t = in.v * (1.0 - (in.s * (1.0 - ff)));

  switch (i) {
  case 0:
    out.r = in.v;
    out.g = t;
    out.b = p;
    break;
  case 1:
    out.r = q;
    out.g = in.v;
    out.b = p;
    break;
  case 2:
    out.r = p;
    out.g = in.v;
    out.b = t;
    break;

  case 3:
    out.r = p;
    out.g = q;
    out.b = in.v;
    break;
  case 4:
    out.r = t;
    out.g = p;
    out.b = in.v;
    break;
  case 5:
  default:
    out.r = in.v;
    out.g = p;
    out.b = q;
    break;
  }
  return RgbColor{static_cast<uint8_t>(out.r * 255),
                  static_cast<uint8_t>(out.g * 255),
                  static_cast<uint8_t>(out.b * 255)};
}

HsvColor rgb2hsv(RgbColor in8) {
  HsvColorF out;
  RgbColorF in{in8.r / 255., in8.g / 255., in8.b / 255.};
  double min, max, delta;

  min = in.r < in.g ? in.r : in.g;
  min = min < in.b ? min : in.b;

  max = in.r > in.g ? in.r : in.g;
  max = max > in.b ? max : in.b;

  out.v = max; // v
  delta = max - min;
  if (delta < 0.00001) {
    out.s = 0;
    out.h = 0; // undefined, maybe nan?
    return {};
  }
  if (max > 0.0) { // NOTE: if Max is == 0, this divide would cause a crash
    out.s = (delta / max); // s
  } else {
    // if max is 0, then r = g = b = 0
    // s = 0, h is undefined
    out.s = 0.0;
    out.h = NAN; // its now undefined
    return {};
  }
  if (in.r >= max)                 // > is bogus, just keeps compilor happy
    out.h = (in.g - in.b) / delta; // between yellow & magenta
  else if (in.g >= max)
    out.h = 2.0 + (in.b - in.r) / delta; // between cyan & yellow
  else
    out.h = 4.0 + (in.r - in.g) / delta; // between magenta & cyan

  out.h *= 60.0; // degrees

  if (out.h < 0.0)
    out.h += 360.0;

  return HsvColor{static_cast<uint8_t>(out.h / 360.0 * 255.),
                  static_cast<uint8_t>(out.s * 255.0),
                  static_cast<uint8_t>(out.v * 255.0)};
}

EMSCRIPTEN_KEEPALIVE int Sum(int a, int b) { return a + b; }

std::vector<RgbColor> Render(int animation_id, int strip_id, int time_ms,
                             int led_count);

int AnimationCount();

EMSCRIPTEN_BINDINGS(led_module) {
  function("AnimationCount", &AnimationCount);
  function("Sum", &Sum);
  function("Render", &Render);

  value_object<RgbColor>("RgbColor")
      .field("r", &RgbColor::r)
      .field("g", &RgbColor::g)
      .field("b", &RgbColor::b);

  register_vector<RgbColor>("vector<RgbColor>");
}

int random(int max) { return rand() % max; }

int random(int min, int max) { return (rand() % (max - min)) + min; }

namespace Animation {
using SetLedFn = std::function<void(int, uint8_t, uint8_t, uint8_t)>;
using GetLedFn = std::function<void(int, uint8_t &, uint8_t &, uint8_t &)>;

struct AnimationSettings {
  int mCount;
  int mIndex;
};

class Animation {
public:
  Animation();
  virtual ~Animation();
  virtual void Reset(const AnimationSettings &settings);
  virtual void Render(SetLedFn set_led, GetLedFn get_led, uint32_t ms) = 0;

protected:
  AnimationSettings mSettings;
};

class FnAnimation : public Animation {
public:
  using AnimationFn = std::function<void(SetLedFn &, GetLedFn &,
                                         const AnimationSettings &, uint32_t)>;
  FnAnimation(AnimationFn fn);
  ~FnAnimation() override;
  void Reset(const AnimationSettings &settings) override;
  void Render(SetLedFn set_led, GetLedFn get_led, uint32_t ms) override;

protected:
  AnimationFn mFn;
};

} // namespace Animation

namespace Animation {
Animation::Animation() = default;
Animation::~Animation() = default;

void Animation::Reset(const AnimationSettings &settings) {
  mSettings = settings;
}

FnAnimation::FnAnimation(AnimationFn fn) : mFn(std::move(fn)) {}

FnAnimation::~FnAnimation() = default;

void FnAnimation::Reset(const AnimationSettings &settings) {
  Animation::Reset(settings);
}

void FnAnimation::Render(SetLedFn set_led, GetLedFn get_led, uint32_t ms) {
  mFn(set_led, get_led, mSettings, ms);
}

FnAnimation WhiteAnimation =
    FnAnimation([](SetLedFn &set_led, GetLedFn &get_led,
                   const AnimationSettings &settings, uint32_t ms) {
      for (int i = 0; i < settings.mCount; ++i) {
        set_led(i, 255, 255, 255);
      }
    });

FnAnimation StripIdAnimation =
    FnAnimation([](SetLedFn &set_led, GetLedFn &get_led,
                   const AnimationSettings &settings, uint32_t ms) {
      for (int i = 0; i < settings.mCount; ++i) {
        if (i < settings.mIndex * 10 + 5) {
          set_led(i, 255, 255, 255);
        } else {
          set_led(i, 0, 0, 0);
        }
      }
    });

FnAnimation DaylightAnimation =
    FnAnimation([](SetLedFn &set_led, GetLedFn &get_led,
                   const AnimationSettings &settings, uint32_t ms) {
      // compute sun color, etc.
      for (int i = 0; i < settings.mCount; ++i) {
        set_led(i, 255, 255, 255);
      }
    });

FnAnimation RainbowAnimation =
    FnAnimation([](SetLedFn &set_led, GetLedFn &get_led,
                   const AnimationSettings &settings, uint32_t ms) {
      float start = (ms % 5000) / 5000. * 256.0;
      const float step = (256.0 / settings.mCount);
      for (int i = 0; i < settings.mCount; ++i) {
        start += step;
        while (start > 255) {
          start = start - 255;
        }
        auto rgb = hsv2rgb({static_cast<uint8_t>(start), 255, 255});
        set_led(i, rgb.r, rgb.g, rgb.b);
      }
    });

FnAnimation CircleRainbowAnimation =
    FnAnimation([](SetLedFn &set_led, GetLedFn &get_led,
                   const AnimationSettings &settings, uint32_t ms) {
      float start = (((ms % 5000) / 5000.) + (settings.mIndex / 4.0)) * 255.0;
      const float step = ((256.0 / settings.mCount) / 4.0);
      for (int i = 0; i < settings.mCount; ++i) {
        start += step;
        while (start > 255) {
          start = start - 255;
        }
        auto rgb = hsv2rgb({static_cast<uint8_t>(start), 255, 255});
        set_led(i, rgb.r, rgb.g, rgb.b);
      }
    });

FnAnimation SparkleAnimation =
    FnAnimation([](SetLedFn &set_led, GetLedFn &get_led,
                   const AnimationSettings &settings, uint32_t ms) {
      // TODO: lets get way more sparkles, but make more of them dim.

      // each pixel has a random change of sparkling.
      // lets say that we want about one third of the strip to sparkle each
      // second on average.
      // every second, we want N new stars.
      // odds = LED_COUNT * fps / stars per sec
      constexpr int odds = (30 * 600) / (180);

      for (int i = 0; i < settings.mCount; i++) {
        if (random(odds) == 0) {
          // sparkle!
          float hue = random(100) / 100.0; // random color
          float brightness =
              random(20, 100) / 100.0; // all are at least 50% brightness.
          brightness = brightness * brightness * brightness * brightness *
                       brightness * brightness;
          float saturation = random(10, 100) / 100.0; // some color?
          /*if (use_color == false) {
            saturation = 0.0;
            hue = 0.0;
          }*/
          RgbColor color = hsv2rgb({static_cast<uint8_t>(hue * 255),
                                    static_cast<uint8_t>(saturation * 255),
                                    static_cast<uint8_t>(brightness * 255)});
          set_led(i, color.r, color.g, color.b);
          if (brightness > 0.65) {
            // lets glow the neighbors.
            color = hsv2rgb({static_cast<uint8_t>(hue * 255),
                             static_cast<uint8_t>(saturation * 255),
                             static_cast<uint8_t>(brightness * 255 / 4)});
            if (i >= 1) {
              set_led(i - 1, color.r, color.g, color.b);
            }
            if (i < (settings.mCount - 1)) {
              set_led(i + 1, color.r, color.g, color.b);
            }
          }
        } else {
          // decay.
          uint8_t r, g, b;
          get_led(i, r, g, b);
          auto hsv = rgb2hsv({r, g, b});
          if (hsv.v > 0.0) {
            // decay over 5 seconds, from 255 to 0, at 30 hz. 30*5 =150 frames,
            // 255
            hsv.v = std::max(0, hsv.v - 3);
          }
          auto rgb = hsv2rgb(hsv);
          set_led(i, rgb.r, rgb.g, rgb.b);
          // TODO: add decay
          /*RgbColor color = strip.GetPixelColor(i);
          HsbColor hsb = HsbColor(color);
          if (hsb.B > 0.0) {
            hsb.B = std::max(0.0, hsb.B - (1.0 / 40));
          }
          strip.SetPixelColor(i, hsb);*/
        }
      }
    });
} // namespace Animation

std::array AllAnimations = {&Animation::CircleRainbowAnimation,
                            &Animation::StripIdAnimation,
                            &Animation::SparkleAnimation};

EMSCRIPTEN_KEEPALIVE int AnimationCount() { return AllAnimations.size(); }

struct StripHistory {
  std::vector<RgbColor> mLeds;
  int mLastAnimation{0};
};

std::map<int, StripHistory> mStripHistories;

EMSCRIPTEN_KEEPALIVE std::vector<RgbColor>
Render(int animation_id, int strip_id, int time_ms, int led_count) {
  auto &history = mStripHistories[strip_id];
  auto &leds = history.mLeds;
  if (history.mLastAnimation != animation_id) {
    leds.clear();
  }
  history.mLastAnimation = animation_id;
  if (leds.size() < led_count) {
    leds.resize(led_count, {0, 0, 0});
  }

  auto set_led = [&leds](int index, uint8_t r, uint8_t g, uint8_t b) {
    leds.at(index) = {r, g, b};
  };
  auto get_led = [&leds](int index, uint8_t &r, uint8_t &g, uint8_t &b) {
    auto led = leds.at(index);
    r = led.r;
    g = led.g;
    b = led.b;
  };

  Animation::AnimationSettings settings{led_count, strip_id};

  auto &animation = *AllAnimations.at(animation_id);

  animation.Reset(settings);
  animation.Render(set_led, get_led, static_cast<uint32_t>(time_ms));

  return leds;
}