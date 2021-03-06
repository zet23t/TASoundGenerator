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
    int8_t editSamplesCopy[sizeof(editSamples)];
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
    static int isContinousPress(bool focused, bool continous) {
        static uint32_t inputBlockTime;
        if (!continous || !focused) return focused && isReleased(0);
        if (isReleased(0) || !isHeld(0)) return 0;
        if (isPressed(0)) {
            inputBlockTime = Time::millis;
            return 1;
        }
        if (Time::millis - inputBlockTime < 200) return 0;
        inputBlockTime += 250;
        return 1;
    }
    int button(int8_t x, int8_t y, int8_t w, int8_t h, SpriteSheetRect rect, bool focused, bool continous) {
        buffer.drawRect(x,y,w,h)->filledRect(!focused || Time::millis %256 > 128 ? RGB565(128,0,0):RGB565(220,0,0));
        drawCenteredSprite(x+w/2,y+h/2,rect);
        return isContinousPress(focused, continous);
    }
    int button(int8_t x, int8_t y, int8_t w, int8_t h, SpriteSheetRect rect, bool focused) {
        return button(x,y,w,h,rect,focused,false);
    }
    int button(int8_t x, int8_t y, int8_t w, int8_t h, const char* text, bool focused,  bool continous) {
        buffer.drawRect(x,y,w,h)->filledRect(!focused || Time::millis %256 > 128 ? RGB565(128,0,0):RGB565(220,0,0));
        buffer.drawText(text,x,y,w,h+1,0,0,false,FontAsset::font,0,RenderCommandBlendMode::opaque);
        return isContinousPress(focused, continous);
    }
    int button(int8_t x, int8_t y, int8_t w, int8_t h, const char* text, bool focused) {
        return button(x,y,w,h,text,focused, false);
    }
    void drawText(const char *text, int8_t x, int8_t y, int8_t w, int8_t h, int8_t halign, int8_t valign) {
        buffer.drawText(text,x,y,w,h,halign,valign,false,FontAsset::font,0,RenderCommandBlendMode::opaque);
    }
    void drawText(const char *text, int8_t x, int8_t y, int8_t w, int8_t h) {
        drawText(text,x,y,w,h,0,0);
    }


    bool loopEditSample;
    void playEditSample(bool loop);
    void stopEditSample();
    void onPlaybackStop(Sound::SamplePlayback* playback) {
        if (loopEditSample) {
            playEditSample(true);
        }
    }
    void playEditSample(bool loop) {
        stopEditSample();
        loopEditSample = loop;
        Sound::playSample(1,editSamples, sampleCount,100,100,1000)->setOnStopCallback(onPlaybackStop, 0);
    }
    void stopEditSample() {
        loopEditSample = false;
        Sound::stopSample(1);
    }

    namespace UI {
        namespace Element {
            const uint8_t BUTTON_MENU = 1;
            const uint8_t BUTTON_PLAY = 2;
            const uint8_t WINDOW_SAMPLES = 3;
            const uint8_t WINDOW_SAMPLES_ACTIVE = 4;

            const uint8_t MENU_CONTROLLER_ID_MIN = 20;
            const uint8_t MENU_BUTTON_AMP_MORE = 20;
            const uint8_t MENU_BUTTON_AMP_LESS = 21;
            const uint8_t MENU_BUTTON_OK = 22;
            const uint8_t MENU_BUTTON_CANCEL = 23;
            const uint8_t MENU_BUTTON_FRQ_MORE = 24;
            const uint8_t MENU_BUTTON_FRQ_LESS = 25;
            const uint8_t MENU_CONTROLLER_ID_MAX = 25;

            const uint8_t CONTEXT_MENU_CLEAR = 10;
            const uint8_t CONTEXT_MENU_NOISE_ADD = 11;
            const uint8_t CONTEXT_MENU_COPY = 12;
            const uint8_t CONTEXT_MENU_PASTE_SET = 13;
            const uint8_t CONTEXT_MENU_PASTE_ADD = 14;
            const uint8_t CONTEXT_MENU_SET_END = 15;
            const uint8_t CONTEXT_MENU_CANCEL = 16;
        }
        const char* CONTEXT_MENU_TEXT[] = {
            "Clear",
            "Add noise",
            "Copy",
            "Paste (set)",
            "Paste (add)",
            "Set end",
            "Cancel",
        };
        const uint8_t FOCUS_SEQUENCE[] = {
            Element::BUTTON_MENU,
            Element::BUTTON_PLAY,
            Element::WINDOW_SAMPLES,
            Element::BUTTON_MENU,

            Element::CONTEXT_MENU_CLEAR,
            Element::CONTEXT_MENU_SET_END,
            Element::CONTEXT_MENU_NOISE_ADD,
            Element::CONTEXT_MENU_COPY,
            Element::CONTEXT_MENU_PASTE_ADD,
            Element::CONTEXT_MENU_PASTE_SET,
            Element::CONTEXT_MENU_CANCEL,
            Element::CONTEXT_MENU_CLEAR,


            Element::MENU_BUTTON_AMP_LESS,
            Element::MENU_BUTTON_AMP_MORE,
            Element::MENU_BUTTON_FRQ_LESS,
            Element::MENU_BUTTON_FRQ_MORE,
            Element::MENU_BUTTON_CANCEL,
            Element::MENU_BUTTON_OK,
            Element::MENU_BUTTON_AMP_LESS,
        };
        namespace ControllerMode{
            const uint8_t ADD_RANDOM_NOISE = 1;
        }
        #define FOCUS_SEQUENCE_COUNT (sizeof(FOCUS_SEQUENCE))
        int8_t selectedElement = Element::BUTTON_MENU;
        uint16_t cursorPos, cursorStartPos;
        uint32_t blockJoystick;
        uint8_t hasDragged;
        uint8_t controllerAmp, controllerFrq;
        uint8_t controllerMode, controllerSeed;

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
                    for (uint8_t j = i+1; j<FOCUS_SEQUENCE_COUNT; j+=1) {
                        if (selectedElement == FOCUS_SEQUENCE[j]) {
                            selectedElement = FOCUS_SEQUENCE[j-1];
                            return;
                        }
                    }
                    selectedElement = FOCUS_SEQUENCE[(i-1+FOCUS_SEQUENCE_COUNT)%FOCUS_SEQUENCE_COUNT];
                    break;
                }
            }
        }
        void AddRandomNoise(uint16_t from, uint16_t to, uint8_t amp, uint8_t frq) {
            int16_t prev = editSamples[from];
            for(uint16_t i = from; i <= to; i+=1) {
                int16_t s = editSamples[i];
                s += (int16_t)((((int8_t)(Math::randInt() &0xff)*amp)>>8));
                if (s < -126) s = -126;
                if (s > 127) s = 127;
                editSamples[i] = (int8_t)s;
            }
        }
        void drawControllers(int8_t dx, int8_t dy) {
            if (selectedElement < Element::MENU_CONTROLLER_ID_MIN || selectedElement > Element::MENU_CONTROLLER_ID_MAX) {
                return;
            }
            buffer.drawRect(0,48,96,16)->filledRect(RGB565(90,90,128));
            buffer.drawRect(7,48,32,7)->filledRect(RGB565(0,0,0));
            buffer.drawText(stringBuffer.start().putDec(controllerAmp).get(),7,49,32,7,0,0,false,FontAsset::font,0);
            buffer.drawRect(7+controllerAmp * 32 / 255,48,1,7)->filledRect(RGB565(128,180,255));
            buffer.drawRect(56,48,33,7)->filledRect(RGB565(0,0,0));
            buffer.drawRect(56+controllerFrq * 33 / 255,48,1,7)->filledRect(RGB565(128,180,255));
            buffer.drawText(stringBuffer.start().putDec(controllerFrq).get(),56,49,32,7,0,0,false,FontAsset::font,0);
            if (button(0,48,7,7,"-",selectedElement == Element::MENU_BUTTON_AMP_LESS,true)) {
                controllerAmp -=1;
            }
            if (button(39,48,7,7,"+",selectedElement == Element::MENU_BUTTON_AMP_MORE,true)) {
                controllerAmp +=1;
            }
            if  (button(49,48,7,7,"-",selectedElement == Element::MENU_BUTTON_FRQ_LESS, true)) {
                controllerFrq -=1;
            }
            if (button(89,48,7,7,"+",selectedElement == Element::MENU_BUTTON_FRQ_MORE, true)) {
                controllerFrq +=1;
            }
            if (button(66,55,30,9,"OK",selectedElement == Element::MENU_BUTTON_OK)) {
                selectedElement = Element::WINDOW_SAMPLES_ACTIVE;
            }
            memcpy(editSamples, editSamplesCopy, sizeof(editSamples));
            if (button(0,55,30,9,"Cancel",selectedElement == Element::MENU_BUTTON_CANCEL)) {
                selectedElement = Element::WINDOW_SAMPLES_ACTIVE;
            }

            Math::setSeed(17, controllerSeed);
            uint16_t a = cursorPos < cursorStartPos ? cursorPos : cursorStartPos;
            uint16_t b = cursorPos >= cursorStartPos ? cursorPos : cursorStartPos;

            switch (controllerMode) {
            case ControllerMode::ADD_RANDOM_NOISE:
                AddRandomNoise(a,b,controllerAmp, controllerFrq);
                break;
            }
        }

        void showControllers(uint8_t type) {
            controllerSeed = (uint8_t)Time::millis;
            memcpy(editSamplesCopy, editSamples, sizeof(editSamples));
            controllerMode= type;
            selectedElement = Element::MENU_BUTTON_AMP_LESS;
            //printf("%d\n",selectedElement);
        }

        void drawContextMenu(int8_t dx, int8_t dy) {
            if (selectedElement < Element::CONTEXT_MENU_CLEAR || selectedElement > Element::CONTEXT_MENU_CANCEL) return;
            buffer.setOffset(-48,0);
            buffer.drawRect(0,0,48,64)->filledRect(RGB565(40,60,90));
            int i = 0,y = 0;
            buffer.drawRect(0,0,48,8)->filledRect(RGB565(80,100,140));
            drawText("-Selection-",2,0,48-4,8);

            while (i < FOCUS_SEQUENCE_COUNT && FOCUS_SEQUENCE[i] != Element::CONTEXT_MENU_CLEAR)i++;
            while (i < FOCUS_SEQUENCE_COUNT && (y == 0 || FOCUS_SEQUENCE[i] != Element::CONTEXT_MENU_CLEAR)) {
                uint8_t item = FOCUS_SEQUENCE[i];
                const char *str = CONTEXT_MENU_TEXT[item-Element::CONTEXT_MENU_CLEAR];
                if (selectedElement == item) buffer.drawRect(0,y+9,48,7)->filledRect(RGB565(60,80,100));
                drawText(str,2,y+9,48-4,8);
                y+=8;
                i++;
            }
            buffer.setOffset(0,0);
            if (isReleased(0)) {
                uint16_t a = cursorPos < cursorStartPos ? cursorPos : cursorStartPos;
                uint16_t b = cursorPos >= cursorStartPos ? cursorPos : cursorStartPos;
                uint8_t sel = selectedElement;
                selectedElement = Element::WINDOW_SAMPLES_ACTIVE;
                switch (sel) {
                case Element::CONTEXT_MENU_SET_END:
                    sampleCount = cursorStartPos;
                    break;
                case Element::CONTEXT_MENU_CLEAR:
                    for (int i=a;i<=b;i+=1) editSamples[i] = 0;
                    break;
                case Element::CONTEXT_MENU_NOISE_ADD:
                    showControllers(ControllerMode::ADD_RANDOM_NOISE);
                    break;
                }
            }


        }

        void showContextMenu() {
            selectedElement = Element::CONTEXT_MENU_CLEAR;
        }

        void drawSamples(bool focused, int8_t dx, int8_t dy) {
            int16_t xoff = cursorPos - 48;
            if (xoff < 0) xoff = 0;
            if (xoff > sizeof(editSamples) - 96) xoff = sizeof(editSamples) - 96;
            drawText(stringBuffer.start()
                     .putDec(editSamples[cursorPos]).put(" @ ")
                     .putDec(cursorPos).put(':').putDec(cursorPos*1000/FREQUENCY).put("ms").get(),0,55,96,9);
            buffer.setOffset(xoff,0);
            buffer.drawRect(sampleCount,14,1,36)->filledRect(RGB565(35,64,128));
            if (selectedElement == Element::WINDOW_SAMPLES_ACTIVE) {
                if (isReleased(0) && !hasDragged) selectedElement = Element::WINDOW_SAMPLES;
                if (isReleased(1) && !hasDragged) {
                    //sampleCount = cursorPos;
                    showContextMenu();
                }
                buffer.drawRect(0,16,sizeof(editSamples),32)->filledRect(RGB565(0,0,64));
                cursorPos += isHeld(0) ? dx * 8 : dx;
                cursorPos %= sizeof(editSamples);
                if (!isHeld(1) && dx) {
                    cursorStartPos = cursorPos;
                }
                if (cursorStartPos != cursorPos) {
                    uint16_t a = cursorPos < cursorStartPos ? cursorPos : cursorStartPos;
                    uint16_t b = cursorPos >= cursorStartPos ? cursorPos : cursorStartPos;
                    int16_t c = b - a;

                    buffer.drawRect(a,15,c,34)->filledRect(RGB565(32,64,128));
                }
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
            buffer.drawRect(0,32,sizeof(editSamples),1)->filledRect(!focused || Time::millis %256 > 128 ? RGB565(0,0,128) : RGB565(0,0,255));
            buffer.setOffset(0,0);
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
            if (button(96-9,0,9,9,ImageAsset::TextureAtlas_atlas::play.sprites[0], selectedElement == Element::BUTTON_PLAY)) {
                playEditSample(false);
            }

            drawContextMenu(x,y);
            drawControllers(x,y);
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
    }
}
