#include <iostream>
#include <array>
#include <random>
#include <string>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <algorithm>
#include <cmath>
#include <vector>
#include <initializer_list>
#include <fstream>
#include <map>
#include <set>
#include <cstdint>
#include <variant>
#include <type_traits>
#include <cstdlib>
#include <iterator>

using namespace std;

int FRAME_RATE {60};
sf::Vector2u WINDOWS_SIZE {1920, 1080};

enum Scene {
    MAIN_MENU,
    MENU,
    SETTINGS,
    CREDITS,
    GAME
};

Scene scene = MAIN_MENU;

void buttons(sf::RenderWindow &window);
int rollDice();


class SimpleJson
{
    using Object=std::map<std::string,SimpleJson>;
    using Array=std::vector<SimpleJson>;
    std::variant<std::nullptr_t,bool,double,std::string,Array,Object> value;
    static void appendUtf8(std::string& out,std::uint32_t cp)
    {
        if(cp<=0x7f)out.push_back(static_cast<char>(cp));
        else if(cp<=0x7ff){out.push_back(static_cast<char>(0xc0|(cp>>6)));out.push_back(static_cast<char>(0x80|(cp&63)));}
        else if(cp<=0xffff){out.push_back(static_cast<char>(0xe0|(cp>>12)));out.push_back(static_cast<char>(0x80|((cp>>6)&63)));out.push_back(static_cast<char>(0x80|(cp&63)));}
        else{out.push_back(static_cast<char>(0xf0|(cp>>18)));out.push_back(static_cast<char>(0x80|((cp>>12)&63)));out.push_back(static_cast<char>(0x80|((cp>>6)&63)));out.push_back(static_cast<char>(0x80|(cp&63)));}
    }
    class Parser
    {
        const std::string& text;std::size_t pos=0;
        void ws(){while(pos<text.size() && (text[pos]==' '||text[pos]=='\n'||text[pos]=='\r'||text[pos]=='\t'))++pos;}
        char take(){if(pos>=text.size())throw std::runtime_error("Nieoczekiwany koniec JSON");return text[pos++];}
        std::uint32_t hex4()
        {
            std::uint32_t n=0;
            for(int i=0;i<4;++i){char c=take();n<<=4;if(c>='0'&&c<='9')n+=c-'0';else if(c>='a'&&c<='f')n+=c-'a'+10;else if(c>='A'&&c<='F')n+=c-'A'+10;else throw std::runtime_error("Bledny escape Unicode");}
            return n;
        }
        std::string string()
        {
            if(take()!='"')throw std::runtime_error("Oczekiwano tekstu JSON");
            std::string out;
            while(true)
            {
                char c=take();if(c=='"')break;
                if(static_cast<unsigned char>(c)<0x20)throw std::runtime_error("Znak sterujacy w tekscie JSON");
                if(c!='\\'){out.push_back(c);continue;}
                c=take();
                switch(c)
                {
                    case '"':out.push_back('"');break;case '\\':out.push_back('\\');break;case '/':out.push_back('/');break;
                    case 'b':out.push_back('\b');break;case 'f':out.push_back('\f');break;case 'n':out.push_back('\n');break;case 'r':out.push_back('\r');break;case 't':out.push_back('\t');break;
                    case 'u':{auto cp=hex4();if(cp>=0xd800&&cp<=0xdbff){if(take()!='\\'||take()!='u')throw std::runtime_error("Brak pary Unicode");const auto low=hex4();if(low<0xdc00||low>0xdfff)throw std::runtime_error("Bledna para Unicode");cp=0x10000+((cp-0xd800)<<10)+(low-0xdc00);}else if(cp>=0xdc00&&cp<=0xdfff)throw std::runtime_error("Bledny znak Unicode");appendUtf8(out,cp);break;}
                    default:throw std::runtime_error("Bledny escape JSON");
                }
            }
            return out;
        }
        SimpleJson value()
        {
            ws();if(pos>=text.size())throw std::runtime_error("Pusty JSON");
            if(text[pos]=='"')return SimpleJson(string());
            if(text[pos]=='{')
            {
                ++pos;Object object;ws();if(pos<text.size()&&text[pos]=='}'){++pos;return SimpleJson(object);}
                while(true){ws();auto key=string();ws();if(take()!=':')throw std::runtime_error("Brak dwukropka w JSON");if(!object.emplace(key,value()).second)throw std::runtime_error("Powtorzony klucz JSON");ws();char c=take();if(c=='}')break;if(c!=',')throw std::runtime_error("Bledny obiekt JSON");}
                return SimpleJson(object);
            }
            if(text[pos]=='[')
            {
                ++pos;Array array;ws();if(pos<text.size()&&text[pos]==']'){++pos;return SimpleJson(array);}
                while(true){array.push_back(value());ws();char c=take();if(c==']')break;if(c!=',')throw std::runtime_error("Bledna tablica JSON");}
                return SimpleJson(array);
            }
            if(text.compare(pos,4,"true")==0){pos+=4;return SimpleJson(true);}
            if(text.compare(pos,5,"false")==0){pos+=5;return SimpleJson(false);}
            if(text.compare(pos,4,"null")==0){pos+=4;return SimpleJson(nullptr);}
            const auto begin=text.c_str()+pos;char* end=nullptr;const double number=std::strtod(begin,&end);
            if(end==begin)throw std::runtime_error("Bledna wartosc JSON");pos+=static_cast<std::size_t>(end-begin);return SimpleJson(number);
        }
    public:explicit Parser(const std::string& source):text(source){} SimpleJson parse(){auto result=value();ws();if(pos!=text.size())throw std::runtime_error("Dane po koncu JSON");return result;}
    };
public:
    SimpleJson():value(nullptr){}
    SimpleJson(std::nullptr_t v):value(v){}
    SimpleJson(bool v):value(v){}
    SimpleJson(double v):value(v){}
    SimpleJson(std::string v):value(std::move(v)){}
    SimpleJson(Array v):value(std::move(v)){}
    SimpleJson(Object v):value(std::move(v)){}
    static SimpleJson parse(const std::string& source){return Parser(source).parse();}
    const SimpleJson& at(const std::string& key)const{const auto* object=std::get_if<Object>(&value);if(!object)throw std::runtime_error("Oczekiwano obiektu JSON");const auto it=object->find(key);if(it==object->end())throw std::runtime_error("Brak pola JSON: "+key);return it->second;}
    const SimpleJson& at(std::size_t index)const{const auto* array=std::get_if<Array>(&value);if(!array||index>=array->size())throw std::runtime_error("Indeks JSON poza tablica");return (*array)[index];}
    std::size_t size()const{if(const auto* a=std::get_if<Array>(&value))return a->size();throw std::runtime_error("Oczekiwano tablicy JSON");}
    const SimpleJson* begin()const{const auto* a=std::get_if<Array>(&value);if(!a)throw std::runtime_error("Oczekiwano tablicy JSON");return a->data();}
    const SimpleJson* end()const{return begin()+size();}
    template<typename T>T get()const
    {
        if constexpr(std::is_same_v<T,std::string>){if(const auto* p=std::get_if<std::string>(&value))return *p;}
        else if constexpr(std::is_same_v<T,int>){if(const auto* p=std::get_if<double>(&value))return static_cast<int>(*p);}
        throw std::runtime_error("Typ pola JSON jest niepoprawny");
    }
};
class UiAudio
{
    sf::SoundBuffer hoverBuffer;
    sf::SoundBuffer clickBuffer;
    sf::SoundBuffer changeBuffer;
    sf::Sound hoverSound;
    sf::Sound clickSound;
    sf::Sound changeSound;
    int previousTarget = 0;

public:
    UiAudio() : hoverSound(hoverBuffer), clickSound(clickBuffer), changeSound(changeBuffer) {}

    bool load()
    {
        if (!hoverBuffer.loadFromFile("resources/sounds/ui_hover.wav") ||
            !clickBuffer.loadFromFile("resources/sounds/ui_click.wav") ||
            !changeBuffer.loadFromFile("resources/sounds/ui_change.wav"))
            return false;
        hoverSound.setBuffer(hoverBuffer);
        clickSound.setBuffer(clickBuffer);
        changeSound.setBuffer(changeBuffer);
        return true;
    }

    void setVolume(float volume)
    {
        hoverSound.setVolume(volume * 0.35f);
        clickSound.setVolume(volume * 0.65f);
        changeSound.setVolume(volume * 0.65f);
        if (volume <= 0.f)
        {
            hoverSound.stop();
            clickSound.stop();
            changeSound.stop();
        }
    }

    void hover(int target)
    {
        if (target != previousTarget && target != 0)
            hoverSound.play();
        previousTarget = target;
    }

    void click()
    {
        hoverSound.stop();
        clickSound.play();
    }

    void change()
    {
        hoverSound.stop();
        clickSound.stop();
        changeSound.play();
    }
};

class DiceMachine
{
    std::array<sf::Texture, 6> textures;
    sf::SoundBuffer soundBuffer;
    sf::Sound sound;
    sf::Sprite dice;
    sf::Clock rollClock;
    sf::Clock animationClock;
    sf::Clock effectsClock;
    float pull = 0.f;
    float grabOffset = 0.f;
    float flash = 0.f;
    bool dragging = false;
    bool rolling = false;
    int completedRoll = 0;

    sf::Vector2f handlePosition() const
    {
        return {315.f, 400.f + 170.f * pull};
    }

    bool onHandle(sf::Vector2f point) const
    {
        const auto d = point - handlePosition();
        return d.x * d.x + d.y * d.y <= 28.f * 28.f;
    }

    void drag(sf::Vector2f point)
    {
        pull = std::clamp((point.y - grabOffset - 400.f) / 170.f, 0.f, 1.f);
        if (pull >= 0.95f)
        {
            pull = 1.f;
            dragging = false;
            rolling = true;
            flash = 0.f;
            rollClock.restart();
            animationClock.restart();
            sound.stop();
            sound.play();
        }
    }

public:
    DiceMachine() : sound(soundBuffer), dice(textures[0]) {}

    bool load()
    {
        for (int i = 0; i < 6; ++i)
        {
            if (!textures[i].loadFromFile("resources/dice/dice" + std::to_string(i + 1) + ".png"))
                return false;
        }
        if (!soundBuffer.loadFromFile("resources/sounds/dice_roll.wav"))
            return false;
        sound.setBuffer(soundBuffer);
        sound.setVolume(75.f);
        dice.setTexture(textures[0], true);
        return true;
    }

    void handleEvent(const sf::Event& event, const sf::RenderWindow& window)
    {
        if (const auto* press = event.getIf<sf::Event::MouseButtonPressed>())
        {
            const auto point = window.mapPixelToCoords(press->position);
            if (!rolling && pull < 0.01f && press->button == sf::Mouse::Button::Left && onHandle(point))
            {
                dragging = true;
                grabOffset = point.y - handlePosition().y;
            }
        }
        if (const auto* move = event.getIf<sf::Event::MouseMoved>())
        {
            if (dragging)
                drag(window.mapPixelToCoords(move->position));
        }
        if (const auto* release = event.getIf<sf::Event::MouseButtonReleased>())
        {
            if (release->button == sf::Mouse::Button::Left && dragging)
            {
                drag(window.mapPixelToCoords(release->position));
                dragging = false;
            }
        }
        if (event.is<sf::Event::FocusLost>())
            dragging = false;
    }

    int takeResult() {const int result=completedRoll;completedRoll=0;return result;}
    bool busy() const {return rolling || dragging;}
    bool isRolling() const {return rolling;}

    void setVolume(float volume)
    {
        sound.setVolume(volume);
    }

    void update(float dt)
    {
        if (!dragging)
            pull = std::max(0.f, pull - dt * 2.5f);
        flash = std::max(0.f, flash - dt);
        if (rolling)
        {
            const float elapsed = rollClock.getElapsedTime().asSeconds();
            if (elapsed >= 1.5f)
            {
                rolling = false;
                const int result = rollDice();
                completedRoll = result;
                dice.setTexture(textures[result - 1], true);
                flash = 0.7f;
                std::cout << result << '\n';
            }
            else if (animationClock.getElapsedTime().asSeconds() >= 0.045f + 0.18f * (elapsed / 1.5f) * (elapsed / 1.5f))
            {
                dice.setTexture(textures[rollDice() - 1], true);
                animationClock.restart();
            }
        }
        const float time = effectsClock.getElapsedTime().asSeconds();
        const auto size = dice.getTexture().getSize();
        const float pulse = 1.f + flash / 7.f;
        const float shake = rolling ? 5.f * std::max(0.f, 1.f - rollClock.getElapsedTime().asSeconds() / 1.5f) : 0.f;
        dice.setOrigin({size.x / 2.f, size.y / 2.f});
        dice.setScale({200.f * pulse / size.x, 200.f * pulse / size.y});
        dice.setPosition({150.f + std::sin(time * 70.f) * shake, 500.f + std::cos(time * 57.f) * shake});
        dice.setRotation(sf::degrees(rolling ? std::sin(time * 35.f) * 6.f : 0.f));
    }

    void draw(sf::RenderWindow& window)
    {
        auto rectangle = [&](sf::Vector2f position, sf::Vector2f size, sf::Color color)
        {
            sf::RectangleShape shape(size);
            shape.setPosition(position);
            shape.setFillColor(color);
            window.draw(shape);
        };
        auto circle = [&](sf::Vector2f center, float radius, sf::Color color)
        {
            sf::CircleShape shape(radius);
            shape.setOrigin({radius, radius});
            shape.setPosition(center);
            shape.setFillColor(color);
            window.draw(shape);
        };
        const auto handle = handlePosition();
        const bool hovered = onHandle(window.mapPixelToCoords(sf::Mouse::getPosition(window)));
        const auto border = flash > 0.f ? sf::Color(255, 215, 70) : sf::Color(45, 190, 210);
        rectangle({22.f, 347.f}, {356.f, 336.f}, border);
        rectangle({25.f, 350.f}, {350.f, 330.f}, sf::Color(18, 27, 40));
        rectangle({307.f, 398.f}, {16.f, 184.f}, sf::Color(90, 110, 130));
        rectangle({309.f, 400.f}, {12.f, 180.f}, sf::Color(5, 10, 18));
        rectangle({309.f, handle.y}, {12.f, 590.f - handle.y}, sf::Color(180, 195, 210));
        circle({315.f, 590.f}, 23.f, sf::Color(145, 160, 180));
        circle({315.f, 590.f}, 20.f, sf::Color(65, 80, 100));
        window.draw(dice);
        circle(handle, 28.f, sf::Color(255, 130, 150));
        circle(handle, 25.f, rolling ? sf::Color(110, 65, 90) :
               (dragging || hovered ? sf::Color(255, 75, 110) : sf::Color(220, 45, 85)));
        circle({handle.x - 7.f, handle.y - 10.f}, 7.f, sf::Color(255, 220, 230, 180));
        const float progress = rolling ? std::min(1.f, rollClock.getElapsedTime().asSeconds() / 1.5f) : (flash > 0.f ? 1.f : pull);
        rectangle({65.f, 640.f}, {260.f, 8.f}, sf::Color(40, 55, 70));
        rectangle({65.f, 640.f}, {260.f * progress, 8.f}, sf::Color(40, 230, 240));
        const float time = effectsClock.getElapsedTime().asSeconds();
        for (int i = 0; i < 8; ++i)
        {
            const bool lit = flash > 0.f || (rolling && static_cast<int>(time * 18.f) % 8 == i);
            circle({57.f + i * 39.f, 370.f}, 5.f, lit ? sf::Color(255, 220, 80) : sf::Color(50, 70, 85));
        }
    }
};

static const std::array<sf::Color, 6> ponyPalette{
    sf::Color(40,220,240), sf::Color(255,90,160), sf::Color(145,110,255),
    sf::Color(110,235,100), sf::Color(255,185,50), sf::Color(245,245,255)
};

static void drawCyberPony(sf::RenderTarget& target, sf::Vector2f position, sf::Color color, float size = 44.f)
{
    sf::Transform transform;
    transform.translate(position);
    transform.scale({size/60.f,size/60.f});
    sf::RenderStates states(transform);
    auto poly=[&](std::initializer_list<sf::Vector2f> points,sf::Color fill)
    {
        sf::ConvexShape shape(points.size());
        std::size_t n=0;
        for(const auto& point:points) shape.setPoint(n++,point);
        shape.setFillColor(fill);
        shape.setOutlineColor(sf::Color(10,20,35));
        shape.setOutlineThickness(1.6f);
        target.draw(shape,states);
    };
    sf::CircleShape shadow(25.f);
    shadow.setScale({1.f,0.3f});shadow.setPosition({-25.f,17.f});
    shadow.setFillColor(sf::Color(0,0,0,120));target.draw(shadow,states);
    poly({{-22.f,-6.f},{-31.f,-1.f},{-32.f,16.f},{-24.f,9.f},{-19.f,1.f}},sf::Color(80,65,155));
    poly({{-17.f,5.f},{-7.f,7.f},{-9.f,24.f},{-18.f,24.f}},color);
    poly({{7.f,4.f},{17.f,4.f},{21.f,24.f},{12.f,24.f}},color);
    poly({{-23.f,-10.f},{5.f,-13.f},{20.f,-2.f},{14.f,12.f},{-19.f,11.f}},color);
    poly({{5.f,-10.f},{8.f,-28.f},{19.f,-32.f},{29.f,-25.f},{27.f,-13.f},{17.f,-10.f},{17.f,3.f}},color);
    poly({{8.f,-26.f},{7.f,-39.f},{16.f,-31.f}},color);
    poly({{19.f,-31.f},{26.f,-43.f},{25.f,-27.f}},sf::Color(180,245,255));
    poly({{7.f,-28.f},{-1.f,-23.f},{-3.f,-7.f},{7.f,-12.f}},sf::Color(75,55,145));
    poly({{14.f,-25.f},{27.f,-24.f},{25.f,-18.f},{14.f,-18.f}},sf::Color(10,35,60));
    poly({{18.f,-24.f},{25.f,-24.f},{24.f,-21.f},{18.f,-21.f}},sf::Color(170,255,255));
    poly({{-12.f,-7.f},{-4.f,-7.f},{0.f,0.f},{-7.f,5.f},{-14.f,0.f}},sf::Color(20,40,65));
    poly({{-9.f,-5.f},{-6.f,-5.f},{-4.f,0.f},{-8.f,2.f}},sf::Color::White);
}

class PlayerMenu
{
    sf::Font font;
    std::array<sf::String, 4> names;
    std::array<int, 4> colors{0,1,2,3};
    int count = 2;
    int active = 0;
    sf::String error;
    sf::Clock cursorClock;

    static sf::FloatRect countBox(int index)
    {
        return {{660.f + index * 210.f, 280.f}, {180.f, 65.f}};
    }

    static sf::FloatRect nameBox(int index)
    {
        return {{760.f, 390.f + index * 85.f}, {375.f, 60.f}};
    }

    void text(sf::RenderWindow& window, const sf::String& value, sf::Vector2f position,
              unsigned int size, sf::Color color = sf::Color::White) const
    {
        sf::Text label(font, value, size);
        label.setPosition(position);
        label.setFillColor(color);
        window.draw(label);
    }

    static void box(sf::RenderWindow& window, sf::FloatRect bounds, sf::Color fill, sf::Color border)
    {
        sf::RectangleShape shape(bounds.size);
        shape.setPosition(bounds.position);
        shape.setFillColor(fill);
        shape.setOutlineThickness(2.f);
        shape.setOutlineColor(border);
        window.draw(shape);
    }

    bool start(std::vector<sf::String>& players)
    {
        std::vector<sf::String> selected;
        for (int i = 0; i < count; ++i)
        {
            sf::String name = names[i];
            while (!name.isEmpty() && name[0] == U' ')
                name.erase(0, 1);
            while (!name.isEmpty() && name[name.getSize() - 1] == U' ')
                name.erase(name.getSize() - 1, 1);
            if (name.isEmpty())
                name = sf::String("Gracz " + std::to_string(i + 1));
            if (std::find(selected.begin(), selected.end(), name) != selected.end())
            {
                error = L"Nicki graczy musz\u0105 by\u0107 r\u00f3\u017cne.";
                active = i;
                return false;
            }
            selected.push_back(name);
        }
        players = selected;
        error.clear();
        return true;
    }

public:
    bool load()
    {
        if (font.openFromFile("C:/Windows/Fonts/arial.ttf"))
            return true;
        return font.openFromFile("resources/fonts/menu.ttf");
    }

    void handleEvent(const sf::Event& event, const sf::RenderWindow& window,
                     std::vector<sf::String>& players)
    {
        if (const auto* click = event.getIf<sf::Event::MouseButtonPressed>())
        {
            if (click->button != sf::Mouse::Button::Left)
                return;
            const auto point = window.mapPixelToCoords(click->position);
            for (int i = 0; i < 3; ++i)
            {
                if (countBox(i).contains(point))
                {
                    count = i + 2;
                    for(int a=0;a<count;++a)
                        for(int b=0;b<a;++b)
                            if(colors[a]==colors[b])
                            {
                                for(int c=0;c<6;++c)
                                {
                                    bool used=false;
                                    for(int k=0;k<a;++k) if(colors[k]==c) used=true;
                                    if(!used) {colors[a]=c;break;}
                                }
                            }
                    active = std::min(active, count - 1);
                    error.clear();
                    return;
                }
            }
            for (int i=0;i<count;++i)
            {
                if (sf::FloatRect({1150.f,390.f+i*85.f},{110.f,60.f}).contains(point))
                {
                    for (int attempt=0;attempt<6;++attempt)
                    {
                        colors[i]=(colors[i]+1)%6;
                        bool used=false;
                        for(int j=0;j<count;++j) if(j!=i && colors[j]==colors[i]) used=true;
                        if(!used) break;
                    }
                    return;
                }
            }
            active = -1;
            for (int i = 0; i < count; ++i)
            {
                if (nameBox(i).contains(point))
                {
                    active = i;
                    cursorClock.restart();
                    return;
                }
            }
            if (sf::FloatRect({660.f, 805.f}, {230.f, 65.f}).contains(point))
                scene = MAIN_MENU;
            else if (sf::FloatRect({940.f, 805.f}, {320.f, 65.f}).contains(point) && start(players))
                scene = GAME;
        }
        if (const auto* input = event.getIf<sf::Event::TextEntered>())
        {
            if (active >= 0 && active < count)
            {
                const char32_t ch = input->unicode;
                if (ch == 8 && !names[active].isEmpty())
                    names[active].erase(names[active].getSize() - 1, 1);
                else if (ch >= 32 && ch != 127 && !(ch >= 0x80 && ch <= 0x9f) && names[active].getSize() < 16)
                    names[active] += sf::String(ch);
                error.clear();
                cursorClock.restart();
            }
        }
        if (const auto* key = event.getIf<sf::Event::KeyPressed>())
        {
            if (key->code == sf::Keyboard::Key::Tab)
                active = key->shift ? (active <= 0 ? count - 1 : active - 1) : (active + 1) % count;
            else if (key->code == sf::Keyboard::Key::Enter && start(players))
                scene = GAME;
            else if (key->code == sf::Keyboard::Key::Escape)
                scene = MAIN_MENU;
        }
    }

    void draw(sf::RenderWindow& window) const
    {
        const sf::Color cyan(45, 210, 225);
        const auto mouse = window.mapPixelToCoords(sf::Mouse::getPosition(window));
        box(window, {{580.f, 130.f}, {760.f, 800.f}}, sf::Color(18, 27, 40), cyan);
        text(window, "NOWA GRA", {660.f, 170.f}, 38, cyan);
        text(window, "Liczba graczy", {660.f, 235.f}, 24);
        for (int i = 0; i < 3; ++i)
        {
            const bool selected = count == i + 2;
            const auto bounds = countBox(i);
            box(window, bounds, selected ? sf::Color(25, 100, 120) : sf::Color(30, 45, 60),
                selected || bounds.contains(mouse) ? cyan : sf::Color(65, 85, 100));
            text(window, std::to_string(i + 2), bounds.position + sf::Vector2f(75.f, 10.f), 30);
        }
        for (int i = 0; i < count; ++i)
        {
            const auto bounds = nameBox(i);
            box(window, {{1150.f,390.f+i*85.f},{110.f,60.f}}, sf::Color(25,35,55), ponyPalette[colors[i]]);
            drawCyberPony(window,{1205.f,430.f+i*85.f},ponyPalette[colors[i]],48.f);
            text(window, "Gracz " + std::to_string(i + 1), {660.f, bounds.position.y + 17.f}, 21);
            box(window, bounds, sf::Color(10, 18, 28), active == i ? cyan : sf::Color(65, 85, 100));
            const bool empty = names[i].isEmpty();
            const sf::String displayed = empty ? sf::String("Wpisz nick...") : names[i];
            text(window, displayed, bounds.position + sf::Vector2f(14.f, 13.f), 25,
                empty ? sf::Color(115, 135, 150) : sf::Color::White);
            if (active == i && static_cast<int>(cursorClock.getElapsedTime().asSeconds() * 2.f) % 2 == 0)
            {
                sf::Text measure(font, names[i], 25);
                measure.setPosition(bounds.position + sf::Vector2f(14.f, 13.f));
                sf::RectangleShape caret({2.f, 30.f});
                caret.setPosition({bounds.position.x + 14.f + measure.getLocalBounds().size.x, bounds.position.y + 13.f});
                caret.setFillColor(cyan);
                window.draw(caret);
            }
        }
        text(window, L"Do 16 znak\u00f3w. Tab: kolejne pole. Kliknij kucyka, aby zmieni\u0107 kolor.", {660.f, 740.f}, 18, sf::Color(150, 175, 190));
        text(window, error, {660.f, 770.f}, 18, sf::Color(255, 120, 140));
        const sf::FloatRect back({660.f, 805.f}, {230.f, 65.f});
        const sf::FloatRect next({940.f, 805.f}, {320.f, 65.f});
        box(window, back, sf::Color(30, 45, 60), back.contains(mouse) ? cyan : sf::Color(65, 85, 100));
        box(window, next, next.contains(mouse) ? sf::Color(30, 135, 145) : sf::Color(20, 95, 110), cyan);
        text(window, "Wstecz", {725.f, 820.f}, 26);
        text(window, L"Rozpocznij gr\u0119", {1000.f, 820.f}, 26);
    }

    int hitTarget(Scene current, sf::Vector2f point) const
    {
        if (current == MENU)
        {
            for (int i = 0; i < 3; ++i)
                if (countBox(i).contains(point))
                    return 10 + i;
            for (int i=0;i<count;++i)
                if (sf::FloatRect({1150.f,390.f+i*85.f},{110.f,60.f}).contains(point)) return 70+i;
            for (int i = 0; i < count; ++i)
                if (nameBox(i).contains(point))
                    return 20 + i;
            if (sf::FloatRect({940.f, 805.f}, {320.f, 65.f}).contains(point))
                return 30;
        }
        if (current == SETTINGS)
        {
            if (sf::FloatRect({660.f, 360.f}, {80.f, 65.f}).contains(point))
                return 40;
            if (sf::FloatRect({1180.f, 360.f}, {80.f, 65.f}).contains(point))
                return 41;
            if (sf::FloatRect({660.f, 455.f}, {600.f, 60.f}).contains(point))
                return 42;
            for (int i = 0; i < 3; ++i)
                if (sf::FloatRect({660.f + i * 210.f, 600.f}, {180.f, 65.f}).contains(point))
                    return 43 + i;
        }
        if ((current == MENU || current == SETTINGS || current == CREDITS) &&
            sf::FloatRect({660.f, 805.f}, {230.f, 65.f}).contains(point))
            return 50 + static_cast<int>(current);
        return 0;
    }

    void handleExtraEvent(const sf::Event& event, sf::RenderWindow& window, float& volume, int& fps)
    {
        if (const auto* key = event.getIf<sf::Event::KeyPressed>())
        {
            if (key->code == sf::Keyboard::Key::Escape)
                scene = MAIN_MENU;
        }
        const auto* click = event.getIf<sf::Event::MouseButtonPressed>();
        if (!click || click->button != sf::Mouse::Button::Left)
            return;
        const auto point = window.mapPixelToCoords(click->position);
        if (sf::FloatRect({660.f, 805.f}, {230.f, 65.f}).contains(point))
        {
            scene = MAIN_MENU;
            return;
        }
        if (scene != SETTINGS)
            return;
        if (sf::FloatRect({660.f, 360.f}, {80.f, 65.f}).contains(point))
            volume = std::max(0.f, volume - 10.f);
        else if (sf::FloatRect({1180.f, 360.f}, {80.f, 65.f}).contains(point))
            volume = std::min(100.f, volume + 10.f);
        else if (sf::FloatRect({660.f, 455.f}, {600.f, 60.f}).contains(point))
            volume = volume > 0.f ? 0.f : 75.f;
        for (int i = 0; i < 3; ++i)
        {
            if (sf::FloatRect({660.f + i * 210.f, 600.f}, {180.f, 65.f}).contains(point))
            {
                fps = std::array<int, 3>{30, 60, 120}[i];
                window.setFramerateLimit(static_cast<unsigned int>(fps));
            }
        }
    }

    void drawExtra(sf::RenderWindow& window, float volume, int fps) const
    {
        const sf::Color cyan(45, 210, 225);
        const auto mouse = window.mapPixelToCoords(sf::Mouse::getPosition(window));
        box(window, {{580.f, 130.f}, {760.f, 800.f}}, sf::Color(18, 27, 40), cyan);
        auto button = [&](sf::FloatRect bounds, const sf::String& caption, bool selected = false)
        {
            box(window, bounds, selected ? sf::Color(25, 100, 120) : sf::Color(30, 45, 60),
                selected || bounds.contains(mouse) ? cyan : sf::Color(65, 85, 100));
            sf::Text label(font, caption, 25);
            const auto textBounds = label.getLocalBounds();
            label.setPosition(bounds.position + (bounds.size - textBounds.size) / 2.f - textBounds.position);
            window.draw(label);
        };
        if (scene == SETTINGS)
        {
            text(window, "USTAWIENIA", {660.f, 180.f}, 38, cyan);
            text(window, L"G\u0142o\u015bno\u015b\u0107 efekt\u00f3w", {660.f, 295.f}, 28);
            button({{660.f, 360.f}, {80.f, 65.f}}, "-");
            button({{1180.f, 360.f}, {80.f, 65.f}}, "+");
            text(window, std::to_string(static_cast<int>(volume)) + "%", {925.f, 372.f}, 30);
            button({{660.f, 455.f}, {600.f, 60.f}}, volume > 0.f ? sf::String(L"Wycisz d\u017awi\u0119k") : sf::String(L"W\u0142\u0105cz d\u017awi\u0119k"), volume == 0.f);
            text(window, "Limit FPS", {660.f, 545.f}, 28);
            const std::array<int, 3> options{30, 60, 120};
            for (int i = 0; i < 3; ++i)
                button({{660.f + i * 210.f, 600.f}, {180.f, 65.f}}, std::to_string(options[i]), fps == options[i]);
            text(window, L"Ustawienia obowi\u0105zuj\u0105 do zamkni\u0119cia gry.", {660.f, 715.f}, 21, sf::Color(150, 175, 190));
        }
        else
        {
            text(window, "AUTORZY", {660.f, 180.f}, 38, cyan);
            text(window, "CYBER KUCYKI", {660.f, 300.f}, 31, sf::Color(150, 175, 190));
            text(window, "Eryk Goraj", {660.f, 415.f}, 48);
            text(window, L"Programowanie, design i muzyka", {660.f, 495.f}, 27, cyan);
        }
        button({{660.f, 805.f}, {230.f, 65.f}}, "Wstecz");
        text(window, L"Escape: powr\u00f3t do menu", {940.f, 825.f}, 20, sf::Color(150, 175, 190));
    }

    std::array<int,4> selectedColors() const {return colors;}

    void drawPlayers(sf::RenderWindow& window, const std::vector<sf::String>& players) const
    {
        text(window, "GRACZE", {35.f, 60.f}, 25, sf::Color(45, 210, 225));
        for (std::size_t i = 0; i < players.size(); ++i)
            text(window, sf::String(std::to_string(i + 1) + ". ") + players[i],
                 {35.f, 105.f + static_cast<float>(i) * 43.f}, 23);
    }
};

struct TrackCell {sf::Vector2f position;std::string tile;int next;};

class BoardColumn
{
    enum Tile { Start, Media, Person, Null1, Bot, Emotion, Blue, Skull };
    sf::RenderTexture spiralTexture{sf::Vector2u{540u, 540u}};
    std::array<sf::Texture, 6> textures;
    const std::array<Tile, 12> leftColumn{
        Start, Media, Person, Null1, Media, Null1,
        Bot, Emotion, Bot, Media, Null1, Emotion
    };
    const std::array<Tile, 11> bottomRow{
        Null1, Media, Bot, Emotion, Bot, Person,
        Media, Null1, Person, Media, Emotion
    };

    const std::array<Tile, 11> topRow{
        Media, Person, Null1, Media, Person, Bot,
        Emotion, Bot, Media, Null1, Emotion
    };

    const std::array<Tile, 10> rightColumn{
        Media, Person, Null1, Media, Person,
        Bot, Emotion, Bot, Media, Null1
    };

    const std::array<Tile, 8> innerBottom{
        Blue, Media, Null1, Person, Media, Bot, Emotion, Person
    };
    const std::array<Tile, 8> innerRight{
        Person, Media, Null1, Person, Media, Bot, Blue, Blue
    };
    const std::array<Tile, 8> innerTop{
        Blue, Null1, Person, Media, Bot, Emotion, Bot, Person
    };
    const std::array<Tile, 8> innerLeft{
        Person, Media, Skull, Person, Emotion, Bot, Emotion, Blue
    };

    void drawTile(sf::RenderTarget& window, Tile type, sf::Vector2f position) const
    {
        if (type == Blue || type == Skull)
        {
            sf::RectangleShape tile({90.f, 90.f});
            tile.setPosition(position);
            tile.setFillColor(type == Blue ? sf::Color(182, 242, 252) : sf::Color(195, 207, 216));
            window.draw(tile);
            sf::RectangleShape border({84.f, 84.f});
            border.setPosition(position + sf::Vector2f(3.f, 3.f));
            border.setFillColor(sf::Color::Transparent);
            border.setOutlineThickness(2.f);
            border.setOutlineColor(type == Blue ? sf::Color(60, 145, 165) : sf::Color(100, 120, 135));
            window.draw(border);
            if (type == Skull)
            {
                auto circle = [&](sf::Vector2f offset, float radius, sf::Color color)
                {
                    sf::CircleShape shape(radius, 40);
                    shape.setPosition(position + offset);
                    shape.setFillColor(color);
                    window.draw(shape);
                };
                auto rect = [&](sf::Vector2f offset, sf::Vector2f size, sf::Color color)
                {
                    sf::RectangleShape shape(size);
                    shape.setPosition(position + offset);
                    shape.setFillColor(color);
                    window.draw(shape);
                };
                const sf::Color bone(245, 246, 230);
                const sf::Color dark(15, 20, 25);
                circle({16.f, 10.f}, 29.f, dark);
                rect({28.f, 45.f}, {34.f, 34.f}, dark);
                circle({20.f, 14.f}, 25.f, bone);
                rect({32.f, 48.f}, {26.f, 27.f}, bone);
                circle({25.f, 33.f}, 9.f, dark);
                circle({47.f, 33.f}, 9.f, dark);
                sf::ConvexShape nose(3);
                nose.setPoint(0, {45.f, 47.f});
                nose.setPoint(1, {39.f, 58.f});
                nose.setPoint(2, {51.f, 58.f});
                nose.setPosition(position);
                nose.setFillColor(dark);
                window.draw(nose);
                rect({39.f, 65.f}, {3.f, 10.f}, dark);
                rect({48.f, 65.f}, {3.f, 10.f}, dark);
            }
            return;
        }
        sf::Sprite sprite(textures[type]);
        const auto size = textures[type].getSize();
        sprite.setScale({90.f / size.x, 90.f / size.y});
        sprite.setPosition(position);
        window.draw(sprite);
    }

    void buildSpiral()
    {
        spiralTexture.clear(sf::Color::Transparent);
        const sf::Color gray(195, 207, 216);
        const sf::Color black(8, 12, 16);
        auto rect = [&](float x, float y, float w, float h, sf::Color color)
        {
            sf::RectangleShape shape({w, h});
            shape.setPosition({x, y});
            shape.setFillColor(color);
            spiralTexture.draw(shape);
        };
        auto sector = [&](sf::Vector2f center, float radius, float start, float sweep)
        {
            sf::ConvexShape shape(34);
            shape.setPoint(0, center);
            for (std::size_t i = 0; i <= 32; ++i)
            {
                const float angle = (start + sweep * static_cast<float>(i) / 32.f) * 3.14159265f / 180.f;
                shape.setPoint(i + 1, center + sf::Vector2f(std::cos(angle), std::sin(angle)) * radius);
            }
            shape.setFillColor(black);
            spiralTexture.draw(shape);
        };
        rect(90.f, 0.f, 450.f, 90.f, gray);
        rect(0.f, 90.f, 90.f, 360.f, gray);
        sector({90.f, 90.f}, 90.f, 180.f, 90.f);
        sector({90.f, 450.f}, 90.f, 90.f, 90.f);
        drawTile(spiralTexture, Emotion, {90.f, 450.f});
        drawTile(spiralTexture, Bot, {180.f, 450.f});
        drawTile(spiralTexture, Person, {270.f, 450.f});
        drawTile(spiralTexture, Media, {360.f, 450.f});
        sector({450.f, 450.f}, 90.f, 0.f, 90.f);
        rect(450.f, 210.f, 90.f, 240.f, gray);
        sector({450.f, 210.f}, 90.f, 270.f, 90.f);
        rect(210.f, 120.f, 240.f, 90.f, gray);
        sector({210.f, 210.f}, 90.f, 180.f, 90.f);
        rect(120.f, 210.f, 90.f, 120.f, gray);
        sector({210.f, 330.f}, 90.f, 90.f, 90.f);
        drawTile(spiralTexture, Skull, {210.f, 330.f});
        sector({300.f, 330.f}, 90.f, 0.f, 90.f);
        sector({300.f, 330.f}, 90.f, 270.f, 90.f);

        sf::ConvexShape arrow(7);
        arrow.setPoint(0, {242.f, 405.f});
        arrow.setPoint(1, {257.f, 405.f});
        arrow.setPoint(2, {257.f, 427.f});
        arrow.setPoint(3, {270.f, 427.f});
        arrow.setPoint(4, {249.f, 447.f});
        arrow.setPoint(5, {229.f, 427.f});
        arrow.setPoint(6, {242.f, 427.f});
        arrow.setFillColor(sf::Color::White);
        spiralTexture.draw(arrow);

        sf::RectangleShape pole({5.f, 108.f});
        pole.setPosition({253.f, 315.f});
        pole.setRotation(sf::degrees(-18.f));
        pole.setOrigin({2.5f, 108.f});
        pole.setFillColor(black);
        spiralTexture.draw(pole);
        sf::Transform flagTransform;
        flagTransform.translate({219.f, 212.f});
        flagTransform.rotate(sf::degrees(-18.f));
        for (int row = 0; row < 4; ++row)
        {
            for (int column = 0; column < 6; ++column)
            {
                sf::ConvexShape cell(4);
                const float x = column * 11.f;
                const float y = row * 11.f;
                const float wave1 = 4.f * std::sin(x / 14.f);
                const float wave2 = 4.f * std::sin((x + 11.f) / 14.f);
                cell.setPoint(0, {x, y + wave1});
                cell.setPoint(1, {x + 11.f, y + wave2});
                cell.setPoint(2, {x + 11.f, y + 11.f + wave2});
                cell.setPoint(3, {x, y + 11.f + wave1});
                cell.setFillColor((row + column) % 2 == 0 ? black : sf::Color::White);
                spiralTexture.draw(cell, sf::RenderStates(flagTransform));
            }
        }
        spiralTexture.display();
        spiralTexture.setSmooth(true);
    }
    static void drawArrow(sf::RenderTarget& target, sf::Vector2f center, float angle, float length = 55.f)
    {
        sf::ConvexShape arrow(7);
        arrow.setPoint(0, {-length * 0.5f, -4.f});
        arrow.setPoint(1, {length * 0.1f, -4.f});
        arrow.setPoint(2, {length * 0.1f, -13.f});
        arrow.setPoint(3, {length * 0.5f, 0.f});
        arrow.setPoint(4, {length * 0.1f, 13.f});
        arrow.setPoint(5, {length * 0.1f, 4.f});
        arrow.setPoint(6, {-length * 0.5f, 4.f});
        arrow.setPosition(center);
        arrow.setRotation(sf::degrees(angle));
        arrow.setFillColor(sf::Color::White);
        target.draw(arrow);
    }

    static void drawBridge(sf::RenderTarget& target, sf::Vector2f center, float angle, float scale = 1.f)
    {
        sf::Transform transform;
        transform.translate(center);
        transform.rotate(sf::degrees(angle));
        transform.scale({scale, scale});
        const sf::RenderStates states(transform);
        auto polygon = [&](std::initializer_list<sf::Vector2f> points, sf::Color color)
        {
            sf::ConvexShape shape(points.size());
            std::size_t i = 0;
            for (const auto& point : points)
                shape.setPoint(i++, point);
            shape.setFillColor(color);
            shape.setOutlineThickness(1.5f);
            shape.setOutlineColor(sf::Color(45, 25, 15));
            target.draw(shape, states);
        };
        auto beam = [&](sf::Vector2f from, sf::Vector2f to, float width, sf::Color color)
        {
            const auto d = to - from;
            sf::RectangleShape shape({std::sqrt(d.x*d.x+d.y*d.y), width});
            shape.setOrigin({0.f, width/2.f});
            shape.setPosition(from);
            shape.setRotation(sf::degrees(std::atan2(d.y,d.x)*180.f/3.14159265f));
            shape.setFillColor(color);
            target.draw(shape, states);
        };
        polygon({{-65.f,15.f},{-32.f,35.f},{65.f,-3.f},{32.f,-23.f}}, sf::Color(123,76,42));
        polygon({{-32.f,35.f},{65.f,-3.f},{65.f,8.f},{-32.f,46.f}}, sf::Color(90,50,27));
        polygon({{-65.f,15.f},{-32.f,35.f},{-32.f,46.f},{-65.f,26.f}}, sf::Color(110,62,32));
        for (int i=0;i<=10;++i)
        {
            const float t=i/10.f;
            const sf::Vector2f back{-65.f+97.f*t,15.f-38.f*t};
            const sf::Vector2f front{-32.f+97.f*t,35.f-38.f*t};
            beam(back,front,1.4f,sf::Color(58,34,20));
        }
        for (int side=0;side<2;++side)
        {
            const sf::Vector2f begin=side==0 ? sf::Vector2f(-65.f,15.f) : sf::Vector2f(-32.f,35.f);
            for (int i=0;i<=5;++i)
            {
                const float t=i/5.f;
                const auto foot=begin+sf::Vector2f(97.f*t,-38.f*t);
                beam(foot,foot+sf::Vector2f(0.f,-35.f),4.f,sf::Color(171,115,65));
                beam(foot+sf::Vector2f(1.f,0.f),foot+sf::Vector2f(1.f,-35.f),1.f,sf::Color(235,184,113));
            }
            beam(begin+sf::Vector2f(0.f,-35.f),begin+sf::Vector2f(97.f,-73.f),5.f,sf::Color(191,137,81));
            beam(begin+sf::Vector2f(0.f,-21.f),begin+sf::Vector2f(97.f,-59.f),3.f,sf::Color(133,83,45));
        }
    }

    void drawConnections(sf::RenderWindow& window) const
    {
        drawArrow(window, {645.f,855.f}, -90.f, 58.f);
        drawArrow(window, {1275.f,225.f}, 90.f, 58.f);
        drawArrow(window, {1275.f,315.f}, 160.f, 58.f);
        drawArrow(window, {555.f,405.f}, 180.f, 58.f);
        sf::RectangleShape startPatch({58.f,19.f});
        startPatch.setPosition({435.f,1056.f});
        startPatch.setFillColor(sf::Color(169,166,163));
        window.draw(startPatch);
        drawArrow(window, {463.f,1065.f}, 0.f, 48.f);
        drawBridge(window, {555.f,913.f}, 0.f, 1.f);
        drawBridge(window, {1320.f,155.f}, 0.f, 1.f);
        drawBridge(window, {1205.f,337.f}, 0.f, 1.f);
    }
public:
    std::vector<TrackCell> makeTrack() const
    {
        std::vector<TrackCell> result;
        const std::array<std::string,8> names{"start","media","person","null1","bot","emotion","blue","skull"};
        auto add=[&](Tile type,float x,float y) {result.push_back({{x+45.f,y+45.f},names[type],static_cast<int>(result.size())+1});};
        add(Start,420.f,990.f);
        for(std::size_t i=0;i<bottomRow.size();++i)add(bottomRow[i],510.f+i*90.f,990.f);
        for(int i=9;i>=0;--i)add(rightColumn[i],1410.f,90.f+i*90.f);
        for(int i=10;i>=0;--i)add(topRow[i],510.f+i*90.f,0.f);
        for(int i=11;i>=1;--i)add(leftColumn[i],420.f,990.f-i*90.f);
        result.back().next=0;
        return result;
    }

    bool load()
    {
        const std::array<std::string, 6> files{
            "start", "media", "person", "null1", "bot", "emotions"
        };
        for (std::size_t i = 0; i < files.size(); ++i)
            if (!textures[i].loadFromFile("resources/boards/" + files[i] + ".png"))
                return false;
        buildSpiral();
        return true;
    }

    void draw(sf::RenderWindow& window) const
    {
        for (std::size_t i = 0; i < leftColumn.size(); ++i)
            drawTile(window, leftColumn[i], {420.f, 990.f - static_cast<float>(i) * 90.f});
        for (std::size_t i = 0; i < topRow.size(); ++i)
            drawTile(window, topRow[i], {510.f + static_cast<float>(i) * 90.f, 0.f});
        for (std::size_t i = 0; i < rightColumn.size(); ++i)
            drawTile(window, rightColumn[i], {1410.f, 90.f + static_cast<float>(i) * 90.f});
        for (std::size_t i = 0; i < bottomRow.size(); ++i)
            drawTile(window, bottomRow[i], {510.f + static_cast<float>(i) * 90.f, 990.f});
        for (std::size_t i = 0; i < innerBottom.size(); ++i)
            drawTile(window, innerBottom[i], {600.f + static_cast<float>(i) * 90.f, 810.f});
        for (std::size_t i = 1; i < innerRight.size(); ++i)
            drawTile(window, innerRight[i], {1230.f, 810.f - static_cast<float>(i) * 90.f});
        for (std::size_t i = 1; i < innerTop.size(); ++i)
            drawTile(window, innerTop[i], {1230.f - static_cast<float>(i) * 90.f, 180.f});
        for (std::size_t i = 1; i + 1 < innerLeft.size(); ++i)
            drawTile(window, innerLeft[i], {600.f, 180.f + static_cast<float>(i) * 90.f});
        sf::Sprite spiral(spiralTexture.getTexture());
        spiral.setPosition({720.f, 300.f});
        spiral.setScale({480.f / 540.f, 480.f / 540.f});
        window.draw(spiral);
        drawConnections(window);
    }
};
class Match
{
    struct Card {int id;std::string tile;sf::String category;sf::String question;std::array<sf::String,4> answers;int correct;};
    struct Pawn {sf::String name;sf::Color color;int cell=0;sf::Vector2f position;};
    sf::Font font;
    std::vector<Card> cards;
    std::map<std::string,std::vector<int>> bags;
    std::vector<TrackCell> track;
    std::vector<Pawn> pawns;
    std::mt19937 random{std::random_device{}()};
    int turn=0;
    int steps=0;
    float stepTime=0.f;
    sf::Vector2f stepFrom;
    int nextCell=0;
    int shownCard=-1;
    int answer=-1;
    bool moving=false;
    bool waitingRoll=false;
    bool roundDone=false;
    bool finished=false;
    sf::String message;

    static sf::String utf(const std::string& value) {return sf::String::fromUtf8(value.begin(),value.end());}
    sf::Vector2f pawnPoint(int cell,int player) const
    {
        static const std::array<sf::Vector2f,4> offsets{{{-19.f,-14.f},{19.f,-14.f},{-19.f,19.f},{19.f,19.f}}};
        return track[cell].position+offsets[player];
    }
    static sf::FloatRect answerBox(int index) {return {{505.f,350.f+index*112.f},{910.f,96.f}};}
    static sf::FloatRect continueBox() {return {{1065.f,900.f},{350.f,65.f}};}
    void label(sf::RenderTarget& window,const sf::String& value,sf::Vector2f pos,unsigned size,sf::Color color=sf::Color::White) const
    {
        sf::Text text(font,value,size);text.setPosition(pos);text.setFillColor(color);window.draw(text);
    }
    sf::String wrap(const sf::String& value,unsigned size,float width) const
    {
        sf::String result,line,word;
        auto push=[&]()
        {
            if(word.isEmpty()) return;
            const sf::String candidate=line.isEmpty()?word:line+sf::String(" ")+word;
            sf::Text measure(font,candidate,size);
            if(!line.isEmpty() && measure.getLocalBounds().size.x>width)
            {result+=line+sf::String("\n");line=word;}
            else line=candidate;
            word.clear();
        };
        for(const auto ch:value)
        {
            if(ch==U' ' || ch==U'\n') {push();if(ch==U'\n'){result+=line+sf::String("\n");line.clear();}}
            else word+=sf::String(ch);
        }
        push();return result+line;
    }
    void rectangle(sf::RenderTarget& window,sf::FloatRect bounds,sf::Color fill,sf::Color border) const
    {
        sf::RectangleShape r(bounds.size);r.setPosition(bounds.position);r.setFillColor(fill);
        r.setOutlineThickness(2.f);r.setOutlineColor(border);window.draw(r);
    }
    void landed()
    {
        moving=false;
        auto& pawn=pawns[turn];
        const auto& cell=track[pawn.cell];
        std::vector<int> eligible;
        for(std::size_t i=0;i<cards.size();++i) if(cards[i].tile==cell.tile) eligible.push_back(static_cast<int>(i));
        if(!eligible.empty())
        {
            auto& bag=bags[cell.tile];
            if(bag.empty()) {bag=eligible;std::shuffle(bag.begin(),bag.end(),random);}
            shownCard=bag.back();bag.pop_back();answer=-1;
            message=L"Wybierz odpowied\u017a A-D.";
        }
        else
        {
            roundDone=true;
            message=L"Ruch zako\u0144czony. Nast\u0119pny gracz.";
        }
    }
    void nextTurn()
    {
        shownCard=-1;answer=-1;roundDone=false;
        turn=(turn+1)%static_cast<int>(pawns.size());
        message=L"Poci\u0105gnij wajch\u0119, aby rzuci\u0107 kostk\u0105.";
    }

public:
    bool load(const std::string& path)
    {
        if(!font.openFromFile("C:/Windows/Fonts/arial.ttf") && !font.openFromFile("resources/fonts/menu.ttf")) return false;
        try
        {
            std::ifstream input(path);
            if(!input) throw std::runtime_error("Nie mozna otworzyc cards.json");
            const std::string source((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
            const auto data=SimpleJson::parse(source);
            if(data.at("schema_version").get<int>()!=1) throw std::runtime_error("Nieznana wersja cards.json");
            std::map<std::string,std::pair<std::string,sf::String>> categories;
            for(const auto* category=data.at("categories").begin();category!=data.at("categories").end();++category)
            {
                const auto id=category->at("id").get<std::string>();
                if(categories.count(id)) throw std::runtime_error("Powtorzona kategoria");
                categories[id]={category->at("tile").get<std::string>(),utf(category->at("name").get<std::string>())};
            }
            std::set<int> ids;
            for(const auto* entry=data.at("cards").begin();entry!=data.at("cards").end();++entry)
            {
                Card card;
                card.id=entry->at("id").get<int>();
                if(!ids.insert(card.id).second) throw std::runtime_error("Powtorzone ID karty");
                const auto category=categories.at(entry->at("category").get<std::string>());
                card.tile=category.first;card.category=category.second;
                card.question=utf(entry->at("question").get<std::string>());
                if(card.question.isEmpty()) throw std::runtime_error("Puste pytanie");
                for(int i=0;i<4;++i)
                {
                    card.answers[i]=utf(entry->at("answers").at(std::string(1,static_cast<char>('A'+i))).get<std::string>());
                    if(card.answers[i].isEmpty()) throw std::runtime_error("Pusta odpowiedz");
                }
                const auto correct=entry->at("correct_answer").get<std::string>();
                if(correct.size()!=1 || correct[0]<'A' || correct[0]>'D') throw std::runtime_error("Bledna poprawna odpowiedz");
                card.correct=correct[0]-'A';cards.push_back(card);
            }
            if(cards.empty()) throw std::runtime_error("Brak kart");
        }
        catch(const std::exception& e) {std::cerr<<"cards.json: "<<e.what()<<'\n';return false;}
        return true;
    }
    void start(const std::vector<sf::String>& names,const std::array<int,4>& colors,std::vector<TrackCell> cells)
    {
        track=std::move(cells);pawns.clear();
        for(std::size_t i=0;i<names.size();++i)
            pawns.push_back({names[i],ponyPalette[colors[i]],0,pawnPoint(0,static_cast<int>(i))});
        bags.clear();turn=0;steps=0;shownCard=-1;answer=-1;moving=false;waitingRoll=false;roundDone=false;finished=false;
        message=L"Poci\u0105gnij wajch\u0119, aby rzuci\u0107 kostk\u0105.";
    }
    bool canRoll() const {return !pawns.empty() && !moving && !waitingRoll && shownCard<0 && !roundDone && !finished;}
    void rollStarted() {waitingRoll=true;message=L"Losowanie...";}
    void acceptRoll(int result)
    {
        if(!waitingRoll || result<1 || result>6) return;
        waitingRoll=false;moving=true;steps=result;stepTime=0.f;
        stepFrom=pawns[turn].position;nextCell=track[pawns[turn].cell].next;
        message=sf::String(L"Wynik: ")+sf::String(std::to_string(result));
    }
    void update(float dt)
    {
        if(!moving) return;
        stepTime+=dt;
        const float t=std::min(stepTime/0.22f,1.f);
        const auto destination=pawnPoint(nextCell,turn);
        pawns[turn].position=stepFrom+(destination-stepFrom)*(t*t*(3.f-2.f*t));
        if(t<1.f) return;
        pawns[turn].cell=nextCell;
        if(--steps<=0) {landed();return;}
        stepTime=0.f;stepFrom=destination;nextCell=track[nextCell].next;
    }
    bool handleEvent(const sf::Event& event,const sf::RenderWindow& window)
    {
        int selected=-1;bool proceed=false;
        if(const auto* key=event.getIf<sf::Event::KeyPressed>())
        {
            if(key->code==sf::Keyboard::Key::A)selected=0;
            if(key->code==sf::Keyboard::Key::B)selected=1;
            if(key->code==sf::Keyboard::Key::C)selected=2;
            if(key->code==sf::Keyboard::Key::D)selected=3;
            proceed=key->code==sf::Keyboard::Key::Enter;
        }
        if(const auto* click=event.getIf<sf::Event::MouseButtonPressed>())
        {
            if(click->button!=sf::Mouse::Button::Left)return false;
            const auto point=window.mapPixelToCoords(click->position);
            if(shownCard>=0)
            {
                for(int i=0;i<4;++i)if(answerBox(i).contains(point))selected=i;
                proceed=continueBox().contains(point);
            }
            else proceed=sf::FloatRect({35.f,825.f},{340.f,65.f}).contains(point);
        }
        if(shownCard>=0 && answer<0 && selected>=0)
        {answer=selected;return true;}
        if(proceed && ((shownCard>=0 && answer>=0)||roundDone))
        {nextTurn();return true;}
        return false;
    }
    int hover(sf::Vector2f point) const
    {
        if(shownCard>=0)
        {
            if(answer<0) for(int i=0;i<4;++i)if(answerBox(i).contains(point))return 100+i;
            if(answer>=0 && continueBox().contains(point))return 110;
        }
        else if(roundDone && sf::FloatRect({35.f,825.f},{340.f,65.f}).contains(point))return 110;
        return 0;
    }
    void draw(sf::RenderWindow& window) const
    {
        if(pawns.empty())return;
        const sf::Color cyan(45,210,225);
        label(window,L"GRACZE",{35.f,40.f},25,cyan);
        for(std::size_t i=0;i<pawns.size();++i)
        {
            if(static_cast<int>(i)==turn)rectangle(window,{{25.f,84.f+i*52.f},{360.f,49.f}},sf::Color(25,60,75),pawns[i].color);
            drawCyberPony(window,{52.f,116.f+i*52.f},pawns[i].color,31.f);
            label(window,pawns[i].name,{80.f,92.f+i*52.f},22);
            drawCyberPony(window,pawns[i].position,pawns[i].color,32.f);
        }
        label(window,L"Tura: "+pawns[turn].name,{35.f,710.f},23,pawns[turn].color);
        label(window,wrap(message,20,340.f),{35.f,752.f},20);
        if(roundDone)
        {
            rectangle(window,{{35.f,825.f},{340.f,65.f}},sf::Color(20,95,110),cyan);
            label(window,L"Nast\u0119pny gracz",{88.f,843.f},24);
        }
        if(shownCard<0)return;
        const auto& card=cards[shownCard];
        rectangle(window,{{0.f,0.f},{1920.f,1080.f}},sf::Color(0,0,0,185),sf::Color::Transparent);
        rectangle(window,{{465.f,110.f},{990.f,890.f}},sf::Color(15,25,42),cyan);
        label(window,wrap(card.category,23,900.f),{505.f,140.f},23,cyan);
        label(window,L"Odpowiada: "+pawns[turn].name,{505.f,186.f},23,pawns[turn].color);
        label(window,wrap(card.question,29,895.f),{505.f,235.f},29);
        const auto point=window.mapPixelToCoords(sf::Mouse::getPosition(window));
        for(int i=0;i<4;++i)
        {
            sf::Color fill(30,45,65),border(80,105,130);
            if(answer<0 && answerBox(i).contains(point))border=cyan;
            if(answer>=0 && i==card.correct){fill=sf::Color(25,105,65);border=sf::Color(100,240,150);}
            else if(answer==i){fill=sf::Color(115,35,55);border=sf::Color(250,100,130);}
            rectangle(window,answerBox(i),fill,border);
            label(window,wrap(sf::String(std::string(1,static_cast<char>('A'+i))+". ")+card.answers[i],24,865.f),answerBox(i).position+sf::Vector2f(17.f,15.f),24);
        }
        if(answer>=0)
        {
            const sf::String feedback=answer==card.correct ? sf::String(L"Dobra odpowied\u017a!") : sf::String(L"Poprawna odpowied\u017a: ")+sf::String(std::string(1,static_cast<char>('A'+card.correct)));
            label(window,feedback,{505.f,830.f},27,answer==card.correct?sf::Color(100,240,150):sf::Color(255,175,100));
            rectangle(window,continueBox(),sf::Color(20,95,110),cyan);
            label(window,L"Nast\u0119pny gracz",{1130.f,919.f},24);
        }
        else label(window,L"Kliknij odpowied\u017a lub naci\u015bnij A, B, C, D.",{505.f,850.f},22,cyan);
    }
};

int main()
{
    sf::RenderWindow window(sf::VideoMode(WINDOWS_SIZE),"Cyber Kucyki");
    window.setFramerateLimit(FRAME_RATE);

    int button_width = 264;
    int button_height = 68;

    sf::Texture settings_basic_texture("resources/buttons/settings.png");
    sf::Texture settings_pressed_texture("resources/buttons/settings_pressed.png");
    sf::Texture settings_onmouse_texture("resources/buttons/settings_onmouse.png");
    sf::Sprite settings_basic(settings_basic_texture);

    sf::Texture play_basic_texture("resources/buttons/play.png");
    sf::Texture play_onmouse_texture("resources/buttons/play_onmouse.png");
    sf::Texture play_pressed_texture("resources/buttons/play_pressed.png");
    sf::Sprite play_basic(play_basic_texture);

    sf::Texture authors_basic_texture("resources/buttons/authors.png");
    sf::Texture authors_onmouse_texture("resources/buttons/authors_onmouse.png");
    sf::Texture authors_pressed_texture("resources/buttons/authors_pressed.png");
    sf::Sprite authors_basic(authors_basic_texture);

    play_basic.setPosition({WINDOWS_SIZE.x/2.0f - (button_width/2.0f),500.0f});
    settings_basic.setPosition({WINDOWS_SIZE.x/2.0f - (button_width/2.0f),500.0f + 3 * button_height/2});
    authors_basic.setPosition({WINDOWS_SIZE.x/2.0f - (button_width/2.0f),500.0f  + 6 * button_height/2});

    PlayerMenu playerMenu;
    if (!playerMenu.load())
        return 1;
    std::vector<sf::String> players;

    BoardColumn board;
    if (!board.load())
        return 1;

    DiceMachine machine;
    if (!machine.load())
        return 1;
    Match match;
    if(!match.load("cards.json"))
        return 1;
    sf::Clock frameClock;

    sf::Clock clock;
    clock.start();
    bool menuTransition = false;
    Scene pendingScene = MAIN_MENU;
    float volume = 75.f;
    UiAudio uiAudio;
    if (!uiAudio.load())
        return 1;
    uiAudio.setVolume(volume);

    while (window.isOpen())
    {
        const float dt = std::min(frameClock.restart().asSeconds(), 0.05f);
        while (const auto event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }

            if (scene == MAIN_MENU && !menuTransition)
            {
                if (const auto* click = event->getIf<sf::Event::MouseButtonPressed>())
                {
                    if (click->button == sf::Mouse::Button::Left)
                    {
                        const auto point = window.mapPixelToCoords(click->position);
                        pendingScene = MAIN_MENU;
                        if (play_basic.getGlobalBounds().contains(point))
                            pendingScene = MENU;
                        else if (settings_basic.getGlobalBounds().contains(point))
                            pendingScene = SETTINGS;
                        else if (authors_basic.getGlobalBounds().contains(point))
                            pendingScene = CREDITS;
                        if (pendingScene != MAIN_MENU)
                        {
                            uiAudio.click();
                            menuTransition = true;
                            clock.restart();
                        }
                    }
                }
            }
            else if (scene == MENU)
            {
                if (const auto* click = event->getIf<sf::Event::MouseButtonPressed>())
                    if (click->button == sf::Mouse::Button::Left &&
                        playerMenu.hitTarget(scene, window.mapPixelToCoords(click->position)) != 0)
                        uiAudio.click();
                playerMenu.handleEvent(*event, window, players);
                if(scene==GAME)
                    match.start(players,playerMenu.selectedColors(),board.makeTrack());
            }
            else if (scene == SETTINGS || scene == CREDITS)
            {
                const float oldVolume = volume;
                const int oldFps = FRAME_RATE;
                int clickedTarget = 0;
                if (const auto* click = event->getIf<sf::Event::MouseButtonPressed>())
                    if (click->button == sf::Mouse::Button::Left)
                        clickedTarget = playerMenu.hitTarget(scene, window.mapPixelToCoords(click->position));
                playerMenu.handleExtraEvent(*event, window, volume, FRAME_RATE);
                uiAudio.setVolume(volume);
                if (oldVolume != volume || oldFps != FRAME_RATE)
                    uiAudio.change();
                else if (clickedTarget != 0)
                    uiAudio.click();
                machine.setVolume(volume);
            }
            else if (scene == GAME)
            {
                if(match.handleEvent(*event,window))uiAudio.click();
                if(match.canRoll())
                {
                    machine.handleEvent(*event, window);
                    if(machine.isRolling()) match.rollStarted();
                }
                else if(machine.busy()) machine.handleEvent(*event,window);
            }
        }
        if (!window.isOpen())
            break;

        window.clear(sf::Color(30, 30, 40));

        sf::Vector2i mousePixel = sf::Mouse::getPosition(window);
        sf::Vector2f mousePos = window.mapPixelToCoords(mousePixel);
        bool settings_hovered = settings_basic.getGlobalBounds().contains(mousePos);
        bool play_hovered = play_basic.getGlobalBounds().contains(mousePos);
        bool authors_hovered = authors_basic.getGlobalBounds().contains(mousePos);
        int hoveredTarget = 0;
        if (window.hasFocus())
        {
            if (scene == MAIN_MENU && !menuTransition)
                hoveredTarget = play_hovered ? 1 : (settings_hovered ? 2 : (authors_hovered ? 3 : 0));
            else if(scene==GAME)
                hoveredTarget=match.hover(mousePos);
            else
                hoveredTarget = playerMenu.hitTarget(scene, mousePos);
        }
        uiAudio.hover(hoveredTarget);




        if (scene == MAIN_MENU)
        {
            auto updateButton = [&](sf::Sprite& sprite, const sf::Texture& basic,
                                    const sf::Texture& hover, const sf::Texture& pressed,
                                    bool hovered, Scene target)
            {
                sprite.setTexture(menuTransition && pendingScene == target ? pressed :
                                  (hovered ? hover : basic), true);
            };
            updateButton(play_basic, play_basic_texture, play_onmouse_texture, play_pressed_texture, play_hovered, MENU);
            updateButton(settings_basic, settings_basic_texture, settings_onmouse_texture, settings_pressed_texture, settings_hovered, SETTINGS);
            updateButton(authors_basic, authors_basic_texture, authors_onmouse_texture, authors_pressed_texture, authors_hovered, CREDITS);
            window.draw(settings_basic);
            window.draw(play_basic);
            window.draw(authors_basic);
            if (menuTransition && clock.getElapsedTime().asSeconds() >= 0.2f)
            {
                menuTransition = false;
                scene = pendingScene;
            }
        }
        else if (scene == SETTINGS || scene == CREDITS)
        {
            playerMenu.drawExtra(window, volume, FRAME_RATE);
        }
        else if (scene == MENU)
        {
            playerMenu.draw(window);
        }
        else if (scene == GAME)
        {
            machine.update(dt);
            const int rolled=machine.takeResult();
            if(rolled>0)match.acceptRoll(rolled);
            match.update(dt);
            board.draw(window);
            machine.draw(window);
            match.draw(window);
        }
        window.display();
    }
}

int rollDice()
{
    static std::mt19937 generator(std::random_device{}());
    static std::uniform_int_distribution<int> distribution(1, 6);

    return distribution(generator);
}