#include "game_common.h"
#include "lib_sound.h"


TinyScreen display = TinyScreen(TinyScreenPlus);
RenderBuffer<uint16_t,RENDER_COMMAND_COUNT> buffer;
TileMap::SceneRenderer<uint16_t,RENDER_COMMAND_COUNT> renderer;
TileMap::Scene<uint16_t> tilemap;


namespace Time {
    uint32_t millis;
}

namespace Game {

    Texture<uint16_t> atlas;
    Sound::SamplePlayback edit;
    int8_t editSamples[256];
    uint16_t sampleCount = 2;

    RenderCommand<uint16_t>* drawCenteredSprite(int x,int y,SpriteSheetRect rect) {
        return buffer.drawRect(x + rect.offsetX - rect.origWidth / 2,
                               y + rect.offsetY - rect.origHeight / 2,
                               rect.width,rect.height)->sprite(&atlas, rect.x, rect.y);
    }
    RenderCommand<uint16_t>* drawSprite(int x,int y,SpriteSheetRect rect) {
        return buffer.drawRect(x + rect.offsetX,
                               y + rect.offsetY,
                               rect.width,rect.height)->sprite(&atlas, rect.x, rect.y);
    }

    bool isHeld(int id) {
        return Joystick::getButton(id,Joystick::Phase::CURRENT);
    }
    bool wasHeld(int id) {
        return Joystick::getButton(id,Joystick::Phase::PREVIOUS);
    }
    bool isPressed(int id) {
        return Joystick::getButton(id,Joystick::Phase::CURRENT) && !Joystick::getButton(id,Joystick::Phase::PREVIOUS);
    }
    bool isReleased(int id) {
        return !Joystick::getButton(id,Joystick::Phase::CURRENT) && Joystick::getButton(id,Joystick::Phase::PREVIOUS);
    }

    int button(int8_t x, int8_t y, int8_t w, int8_t h, SpriteSheetRect rect, bool focused) {
        buffer.drawRect(x,y,w,h)->filledRect(!focused || Time::millis %256 > 128 ? RGB565(128,0,0):RGB565(220,0,0));
        drawCenteredSprite(x+w/2,y+h/2,rect);
        return isReleased(0);
    }
    int button(int8_t x, int8_t y, int8_t w, int8_t h, const char* text, bool focused) {
        buffer.drawRect(x,y,w,h)->filledRect(!focused || Time::millis %256 > 128 ? RGB565(128,0,0):RGB565(220,0,0));
        buffer.drawText(text,x,y,w,h+1,0,0,false,FontAsset::font,0,RenderCommandBlendMode::opaque);
        return isReleased(0);
    }
    void drawText(const char *text, int8_t x, int8_t y, int8_t w, int8_t h, int8_t halign, int8_t valign) {
        buffer.drawText(text,x,y,w,h,halign,valign,false,FontAsset::font,0,RenderCommandBlendMode::opaque);
    }
    void drawText(const char *text, int8_t x, int8_t y, int8_t w, int8_t h) {
        drawText(text,x,y,w,h,0,0);
    }

    namespace UI {
        namespace Element {
            const uint8_t BUTTON_MENU = 1;
            const uint8_t BUTTON_PLAY = 2;
            const uint8_t WINDOW_SAMPLES = 3;
            const uint8_t WINDOW_SAMPLES_ACTIVE = 4;
        }
        const uint8_t FOCUS_SEQUENCE[] = {
            Element::BUTTON_MENU,
            Element::BUTTON_PLAY,
            Element::WINDOW_SAMPLES,

        };
        #define FOCUS_SEQUENCE_COUNT (sizeof(FOCUS_SEQUENCE))
        int8_t selectedElement = Element::BUTTON_MENU;
        uint16_t cursorPos;
        uint32_t blockJoystick;
        uint8_t hasDragged;


        void nextElement() {
            for (uint8_t i = 0; i < FOCUS_SEQUENCE_COUNT; i+=1) {
                if (selectedElement == FOCUS_SEQUENCE[i]) {
                    selectedElement = FOCUS_SEQUENCE[(i+1)%FOCUS_SEQUENCE_COUNT];
                    break;
                }
            }
        }
        void prevElement() {
            for (uint8_t i = 0; i < FOCUS_SEQUENCE_COUNT; i+=1) {
                if (selectedElement == FOCUS_SEQUENCE[i]) {
                    selectedElement = FOCUS_SEQUENCE[(i-1+FOCUS_SEQUENCE_COUNT)%FOCUS_SEQUENCE_COUNT];
                    break;
                }
            }
        }
        void drawSamples(bool focused, int8_t dx, int8_t dy) {
            drawText(stringBuffer.start()
                     .putDec(editSamples[cursorPos]).put(" @ ")
                     .putDec(cursorPos).put(':').putDec(cursorPos*1000/FREQUENCY).put("ms").get(),0,55,96,9);
            if (selectedElement == Element::WINDOW_SAMPLES_ACTIVE) {
                if (isReleased(0) && !hasDragged) selectedElement = Element::WINDOW_SAMPLES;
                buffer.drawRect(0,16,96,32)->filledRect(RGB565(0,0,64));
                cursorPos += isHeld(0) ? dx * 8 : dx;
                cursorPos %= sizeof(editSamples);
                buffer.drawRect(cursorPos,15,1,34)->filledRect(RGB565(255,255,255));
                editSamples[cursorPos] += isHeld(0) ? dy * 8 : dy;
            } else if (selectedElement == Element::WINDOW_SAMPLES) {
                if (isReleased(0) && !hasDragged) selectedElement = Element::WINDOW_SAMPLES_ACTIVE;
            }
            for (int i=0;i<sizeof(editSamples);i+=1) {
                int8_t s = editSamples[i]>>3;
                int8_t y = 33, h = s < 0 ? -s : s;
                if (h == 0) {
                    continue;
                }
                if (s<0) y = 32 - h;
                buffer.drawRect(i,y,1,h)->filledRect(RGB565(128,128,255));
            }
            buffer.drawRect(0,32,96,1)->filledRect(!focused || Time::millis %256 > 128 ? RGB565(0,0,128) : RGB565(0,0,255));

        }
        void drawUI() {
            Fixed2D4 stick = Joystick::getJoystick();
            int8_t x = 0, y = 0;
            if (stick.x.absolute() < Fix4(0,6) && stick.y.absolute() < Fix4(0,6)) {
                blockJoystick = 0;
            } else if (isHeld(0) || isHeld(1)) {
                hasDragged = 1;
            }
            if (blockJoystick == 0 || Time::millis - blockJoystick > 150) {
                x = stick.x >= Fix4(0,6) ? 1 : (stick.x <= Fix4(-1,10) ? -1 : 0);
                y = stick.y >= Fix4(0,6) ? 1 : (stick.y <= Fix4(-1,10) ? -1 : 0);
                if (x>0 || y > 0) {
                    nextElement();
                }
                if (x < 0 || y < 0) {
                    prevElement();
                }
                if (x||y) {
                    blockJoystick = blockJoystick == 0 ? Time::millis : blockJoystick + 30;

                }
            }
            drawSamples(selectedElement == Element::WINDOW_SAMPLES, x,y);
            button(0,0,24,9,"Menu", selectedElement == Element::BUTTON_MENU);
            button(96-9,0,9,9,ImageAsset::TextureAtlas_atlas::play.sprites[0], selectedElement == Element::BUTTON_PLAY);
            if (!wasHeld(0) && !wasHeld(1)) hasDragged = 0;

        }
    }


    void tick() {
        Sound::tick();
        UI::drawUI();
    }

    void initialize() {
        editSamples[0] = 100;
        editSamples[1] = -100;
        buffer.setClearBackground(true,0);
        Sound::init();
        atlas = Texture<uint16_t>(ImageAsset::atlas);
        Sound::playSample(1,editSamples, 2,100,100,1000);
    }
}