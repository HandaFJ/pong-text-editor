#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <cmath>
#include <random>
#include <iostream>
#include <string>
#include <vector>
#include <cstdint>
#include <algorithm>
#include <optional>

// --- 定数定義 ---
constexpr float FONT_SIZE = 24.0f;
constexpr float LINE_HEIGHT = 32.0f;
constexpr float CHAR_WIDTH = 14.0f; 
constexpr float MARGIN_X = 50.0f;
constexpr float MARGIN_Y = 50.0f;

constexpr float WINDOW_WIDTH = 1280.0f;
constexpr float WINDOW_HEIGHT = 720.0f;
constexpr float MAX_TEXT_WIDTH = 1100.0f;

// 入力制限用: 1行あたりの文字数と、画面に収まる最大行数（折り返し後）
constexpr size_t COLS_PER_ROW = static_cast<size_t>(MAX_TEXT_WIDTH / CHAR_WIDTH);                      // 78
constexpr size_t MAX_ROWS     = static_cast<size_t>((WINDOW_HEIGHT - MARGIN_Y * 2.0f) / LINE_HEIGHT);  // 19

// --- 演出用構造体 ---

struct Particle {
    sf::Vector2f pos;
    sf::Vector2f vel;
    sf::Color color;
    float size;
    float lifetime;
    float maxLifetime;
};

struct PongChar {
    std::string text;
    sf::Vector2f pos;
    sf::Vector2f vel;
    sf::Color color;
    float rotation = 0.0f;
    float rotSpeed = 0.0f;
};

struct PopEffect {
    sf::Vector2f pos;
    std::string text;
    float scale = 2.0f;
    float alpha = 255.0f;
};

class GameEditor {
private:
    sf::RenderWindow window;
    sf::Font font;

    // エディタ状態
    std::vector<std::string> lines;
    size_t cx = 0;
    size_t cy = 0;

    // 演出・物理パラメータ
    sf::Vector2f targetCursorPos;
    sf::Vector2f currentCursorPos;
    float screenShakeTime = 0.0f;
    float screenShakeIntensity = 0.0f;

    std::vector<Particle> particles;
    std::vector<PongChar> pongChars;
    std::vector<PopEffect> popEffects;

    std::mt19937 rng;

    float randomFloat(float min, float max) {
        std::uniform_real_distribution<float> dist(min, max);
        return dist(rng);
    }

    bool hasUserTyped() const {
        if (lines.size() > 1) return true;
        if (!lines.empty() && !lines[0].empty()) return true;
        return false;
    }

    // --- 入力制限 ---

    // 長さ len の論理行が、折り返し後に何行になるか（空行も1行と数える）
    static size_t rowsForLength(size_t len) {
        return len == 0 ? 1 : (len + COLS_PER_ROW - 1) / COLS_PER_ROW;
    }

    size_t totalRows() const {
        size_t total = 0;
        for (const auto& l : lines) total += rowsForLength(l.size());
        return total;
    }

    // 1文字挿入しても画面に収まるか
    bool canInsertChar() const {
        size_t len = lines[cy].size();
        return totalRows() - rowsForLength(len) + rowsForLength(len + 1) <= MAX_ROWS;
    }

    // 現在位置で改行しても画面に収まるか
    bool canInsertNewline() const {
        size_t len = lines[cy].size();
        return totalRows() - rowsForLength(len)
               + rowsForLength(cx) + rowsForLength(len - cx) <= MAX_ROWS;
    }

    // 「これ以上打てない」ときの演出
    void rejectInput() {
        triggerShake(5.0f, 0.12f);
        spawnParticles(targetCursorPos, sf::Color(255, 60, 60), 8);
    }

    // フォント読み込み（Windows & macOS対応）
    void loadFont() {
        const std::vector<std::string> fontCandidates = {
            "assets/ipaexg.ttf",
            "assets/font.ttf",
        #if defined(_WIN32)
                    "C:\\Windows\\Fonts\\ipaexg.ttf",
                    "C:\\Windows\\Fonts\\msgothic.ttc",
                    "C:\\Windows\\Fonts\\meiryo.ttc",
        #elif defined(__APPLE__)
                    "/System/Library/Fonts/Supplemental/Courier New.ttf",
                    "/System/Library/Fonts/Helvetica.ttc",
        #endif
        };

        bool loaded = false;
        for (const auto& path : fontCandidates) {
            if (font.openFromFile(path)) {
                std::cout << "[Info] Successfully loaded font: " << path << std::endl;
                loaded = true;
                break;
            }
        }

        if (!loaded) {
            std::cerr << "[Error] Could not load any default font. Please place 'ipaexg.ttf' in the executable directory." << std::endl;
        }
    }

public:
    GameEditor() 
        : window(sf::VideoMode({static_cast<unsigned int>(WINDOW_WIDTH), static_cast<unsigned int>(WINDOW_HEIGHT)}), 
                 "Pong Text Editor", sf::Style::Close | sf::Style::Titlebar),
          rng(std::random_device{}()) {
        window.setFramerateLimit(60);

        loadFont();

        lines.push_back("");
        updateTargetCursorPos();
        currentCursorPos = targetCursorPos;
    }

    void run() {
        sf::Clock clock;

        while (window.isOpen()) {
            float dt = clock.restart().asSeconds();

            processEvents();
            update(dt);
            render();
        }
    }

private:
    void updateTargetCursorPos() {
        float currentX = MARGIN_X;
        float currentY = MARGIN_Y;
        
        for (size_t i = 0; i <= cy && i < lines.size(); ++i) {
            std::string targetStr = (i == cy) ? lines[i].substr(0, cx) : lines[i];
            
            for (char c : targetStr) {
                if (currentX + CHAR_WIDTH > MARGIN_X + MAX_TEXT_WIDTH) {
                    currentX = MARGIN_X;
                    currentY += LINE_HEIGHT;
                }
                if (i < cy) {
                    currentX += CHAR_WIDTH;
                } else {
                    break;
                }
            }
            if (i < cy) {
                currentX = MARGIN_X;
                currentY += LINE_HEIGHT;
            }
        }

        float rowStartY = MARGIN_Y;
        for(size_t i=0; i<cy; ++i) {
            float rX = MARGIN_X;
            for(char c : lines[i]) {
                if (rX + CHAR_WIDTH > MARGIN_X + MAX_TEXT_WIDTH) { rX = MARGIN_X; rowStartY += LINE_HEIGHT; }
                rX += CHAR_WIDTH;
            }
            rowStartY += LINE_HEIGHT;
        }

        float curX = MARGIN_X;
        std::string sub = lines[cy].substr(0, cx);
        for(char c : sub) {
            if (curX + CHAR_WIDTH > MARGIN_X + MAX_TEXT_WIDTH) {
                curX = MARGIN_X;
                rowStartY += LINE_HEIGHT;
            }
            curX += CHAR_WIDTH;
        }

        targetCursorPos.x = curX;
        targetCursorPos.y = rowStartY;
    }

    void triggerShake(float intensity, float duration) {
        screenShakeIntensity = intensity;
        screenShakeTime = duration;
    }

    void spawnParticles(sf::Vector2f pos, sf::Color color, int count = 10) {
        for (int i = 0; i < count; ++i) {
            float angle = randomFloat(0.0f, 2.0f * 3.14159f);
            float speed = randomFloat(40.0f, 220.0f);
            float life = randomFloat(0.2f, 0.45f);

            Particle p;
            p.pos = pos;
            p.vel = sf::Vector2f(std::cos(angle) * speed, std::sin(angle) * speed);
            p.color = color;
            p.size = randomFloat(2.5f, 6.5f);
            p.lifetime = life;
            p.maxLifetime = life;

            particles.push_back(p);
        }
    }

    void spawnPongChar(std::string ch, sf::Vector2f startPos) {
        PongChar pc;
        pc.text = ch;
        pc.pos = startPos;

        float angle = randomFloat(0.0f, 2.0f * 3.14159f);
        float speed = randomFloat(180.0f, 380.0f);
        pc.vel = sf::Vector2f(std::cos(angle) * speed, std::sin(angle) * speed);

        int colorType = static_cast<int>(randomFloat(0.0f, 3.99f));
        if (colorType == 0) pc.color = sf::Color(100, 220, 255);
        else if (colorType == 1) pc.color = sf::Color(255, 100, 200);
        else if (colorType == 2) pc.color = sf::Color(100, 255, 120);
        else pc.color = sf::Color(255, 220, 80);

        pc.rotSpeed = randomFloat(-200.0f, 200.0f);
        pongChars.push_back(pc);
    }

    void processEvents() {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }

            if (const auto* textEvent = event->getIf<sf::Event::TextEntered>()) {
                std::uint32_t unicode = textEvent->unicode;
                
                // ASCII印字可能文字のみ（1バイトに切り詰めて文字化けするのを防ぐ）
                if (unicode >= 32 && unicode < 127) {
                    // 画面に収まらないなら入力を拒否
                    if (!canInsertChar()) {
                        rejectInput();
                        continue;
                    }

                    std::string& line = lines[cy];
                    std::string insertedChar = "";
                    insertedChar += static_cast<char>(unicode);
                    
                    line.insert(cx, insertedChar);
                    
                    PopEffect pop;
                    pop.pos = targetCursorPos;
                    pop.text = insertedChar;
                    popEffects.push_back(pop);

                    spawnParticles(targetCursorPos + sf::Vector2f(0.0f, LINE_HEIGHT / 2.0f), sf::Color(255, 200, 80));
                    spawnPongChar(insertedChar, targetCursorPos);

                    cx++;
                    updateTargetCursorPos();
                }
            }

            if (const auto* keyEvent = event->getIf<sf::Event::KeyPressed>()) {
                switch (keyEvent->code) {
                case sf::Keyboard::Key::Enter:
                    {
                        // 画面に収まらないなら改行を拒否
                        if (!canInsertNewline()) {
                            rejectInput();
                            break;
                        }

                        std::string currentLine = lines[cy];
                        std::string nextLine = currentLine.substr(cx);
                        lines[cy] = currentLine.substr(0, cx);
                        lines.insert(lines.begin() + cy + 1, nextLine);
                        
                        cy++;
                        cx = 0;
                        updateTargetCursorPos();

                        triggerShake(6.0f, 0.15f);
                        spawnParticles(targetCursorPos, sf::Color(100, 220, 255), 20);
                    }
                    break;

                case sf::Keyboard::Key::Backspace:
                    if (cx > 0) {
                        lines[cy].erase(cx - 1, 1);
                        cx--;
                        updateTargetCursorPos();
                        spawnParticles(targetCursorPos, sf::Color(255, 80, 80), 6);

                        if (!pongChars.empty()) {
                            spawnParticles(pongChars.back().pos, sf::Color(255, 100, 100), 12);
                            pongChars.pop_back();
                        }
                    } else if (cy > 0) {
                        cx = lines[cy - 1].size();
                        lines[cy - 1] += lines[cy];
                        lines.erase(lines.begin() + cy);
                        cy--;
                        updateTargetCursorPos();
                        triggerShake(4.0f, 0.1f);
                    }
                    break;

                case sf::Keyboard::Key::Left:
                    if (cx > 0) cx--;
                    else if (cy > 0) { cy--; cx = lines[cy].size(); }
                    updateTargetCursorPos();
                    break;

                case sf::Keyboard::Key::Right:
                    if (cx < lines[cy].size()) cx++;
                    else if (cx < lines.size() - 1) { cy++; cx = 0; }
                    updateTargetCursorPos();
                    break;

                case sf::Keyboard::Key::Up:
                    if (cy > 0) { cy--; if (cx > lines[cy].size()) cx = lines[cy].size(); }
                    updateTargetCursorPos();
                    break;

                case sf::Keyboard::Key::Down:
                    if (cy < lines.size() - 1) { cy++; if (cx > lines[cy].size()) cx = lines[cy].size(); }
                    updateTargetCursorPos();
                    break;

                default:
                    break;
                }
            }
        }
    }

    void update(float dt) {
        float lerpSpeed = 22.0f;
        currentCursorPos.x += (targetCursorPos.x - currentCursorPos.x) * lerpSpeed * dt;
        currentCursorPos.y += (targetCursorPos.y - currentCursorPos.y) * lerpSpeed * dt;

        if (screenShakeTime > 0.0f) {
            screenShakeTime -= dt;
            if (screenShakeTime <= 0.0f) screenShakeIntensity = 0.0f;
        }

        for (auto it = particles.begin(); it != particles.end();) {
            it->pos += it->vel * dt;
            it->lifetime -= dt;
            float alphaProgress = it->lifetime / it->maxLifetime;
            it->color.a = static_cast<std::uint8_t>(std::max(0.0f, alphaProgress) * 255);
            if (it->lifetime <= 0.0f) it = particles.erase(it);
            else ++it;
        }

        for (size_t i = 0; i < pongChars.size(); ++i) {
            pongChars[i].pos += pongChars[i].vel * dt;
            pongChars[i].rotation += pongChars[i].rotSpeed * dt;

            if (pongChars[i].pos.x < 20.0f) { pongChars[i].pos.x = 20.0f; pongChars[i].vel.x *= -1.0f; }
            if (pongChars[i].pos.x > WINDOW_WIDTH - 30.0f) { pongChars[i].pos.x = WINDOW_WIDTH - 30.0f; pongChars[i].vel.x *= -1.0f; }
            if (pongChars[i].pos.y < 20.0f) { pongChars[i].pos.y = 20.0f; pongChars[i].vel.y *= -1.0f; }
            if (pongChars[i].pos.y > WINDOW_HEIGHT - 30.0f) { pongChars[i].pos.y = WINDOW_HEIGHT - 30.0f; pongChars[i].vel.y *= -1.0f; }
        }

        float collisionRadius = 18.0f;
        for (size_t i = 0; i < pongChars.size(); ++i) {
            for (size_t j = i + 1; j < pongChars.size(); ++j) {
                sf::Vector2f diff = pongChars[j].pos - pongChars[i].pos;
                float distSq = diff.x * diff.x + diff.y * diff.y;

                if (distSq < (collisionRadius * 2.0f) * (collisionRadius * 2.0f)) {
                    sf::Vector2f tempVel = pongChars[i].vel;
                    pongChars[i].vel = pongChars[j].vel;
                    pongChars[j].vel = tempVel;

                    float dist = std::sqrt(distSq);
                    if (dist > 0.0f) {
                        sf::Vector2f normal = diff / dist;
                        float overlap = (collisionRadius * 2.0f) - dist;
                        pongChars[i].pos -= normal * (overlap * 0.5f);
                        pongChars[j].pos += normal * (overlap * 0.5f);
                    }

                    sf::Vector2f midPos = (pongChars[i].pos + pongChars[j].pos) * 0.5f;
                    spawnParticles(midPos, sf::Color(255, 255, 255), 6);
                    triggerShake(2.0f, 0.08f);
                }
            }
        }

        for (auto it = popEffects.begin(); it != popEffects.end();) {
            it->scale += (1.0f - it->scale) * 15.0f * dt;
            it->alpha -= 500.0f * dt;
            if (it->alpha <= 0.0f) it = popEffects.erase(it);
            else ++it;
        }
    }

    void render() {
        sf::View view = window.getDefaultView();
        if (screenShakeIntensity > 0.0f) {
            float offsetX = randomFloat(-screenShakeIntensity, screenShakeIntensity);
            float offsetY = randomFloat(-screenShakeIntensity, screenShakeIntensity);
            view.move({offsetX, offsetY});
        }
        window.setView(view);

        window.clear(sf::Color(18, 20, 26));

        // --- A. 跳ね回るPong文字の描画 ---
        for (const auto& pc : pongChars) {
            sf::Text pongText(font, pc.text, static_cast<unsigned int>(FONT_SIZE));
            pongText.setPosition(pc.pos);
            pongText.setRotation(sf::degrees(pc.rotation));
            pongText.setFillColor(pc.color);
            window.draw(pongText);
        }

        // --- B. プレースホルダーの描画 ---
        if (!hasUserTyped()) {
            sf::Text placeholder(font, "Type something here...", static_cast<unsigned int>(FONT_SIZE));
            placeholder.setPosition({MARGIN_X, MARGIN_Y});
            placeholder.setFillColor(sf::Color(100, 110, 130, 120));
            window.draw(placeholder);
        }

        // --- C. 自動折り返しを考慮したテキスト描画 ---
        sf::Text textDrawable(font, "", static_cast<unsigned int>(FONT_SIZE));
        textDrawable.setFillColor(sf::Color(220, 225, 235, 240));

        float currentY = MARGIN_Y;
        for (size_t i = 0; i < lines.size(); ++i) {
            float currentX = MARGIN_X;
            std::string currentLineChunk = "";

            for (char c : lines[i]) {
                if (currentX + CHAR_WIDTH > MARGIN_X + MAX_TEXT_WIDTH) {
                    textDrawable.setPosition({currentX - currentLineChunk.length() * CHAR_WIDTH, currentY});
                    textDrawable.setString(currentLineChunk);
                    window.draw(textDrawable);

                    currentLineChunk = "";
                    currentX = MARGIN_X;
                    currentY += LINE_HEIGHT;
                }
                currentLineChunk += c;
                currentX += CHAR_WIDTH;
            }

            if (!currentLineChunk.empty() || lines[i].empty()) {
                float startX = MARGIN_X;
                textDrawable.setPosition({startX, currentY});
                textDrawable.setString(currentLineChunk);
                window.draw(textDrawable);
            }
            currentY += LINE_HEIGHT;
        }

        // --- D. パーティクルの描画 ---
        for (const auto& p : particles) {
            sf::CircleShape shape(p.size);
            shape.setOrigin({p.size, p.size});
            shape.setPosition(p.pos);
            shape.setFillColor(p.color);
            window.draw(shape);
        }

        // --- E. ポップアップ文字描画 ---
        for (const auto& pop : popEffects) {
            sf::Text popText(font, pop.text, static_cast<unsigned int>(FONT_SIZE));
            popText.setOrigin({CHAR_WIDTH / 2.0f, FONT_SIZE / 2.0f});
            popText.setPosition({pop.pos.x + CHAR_WIDTH / 2.0f, pop.pos.y + FONT_SIZE / 2.0f});
            popText.setScale({pop.scale, pop.scale});
            popText.setFillColor(sf::Color(255, 230, 100, static_cast<std::uint8_t>(std::max(0.0f, pop.alpha))));
            window.draw(popText);
        }

        // --- F. カーソルの描画 ---
        sf::RectangleShape cursorShape(sf::Vector2f(3.0f, LINE_HEIGHT * 0.85f));
        cursorShape.setPosition({currentCursorPos.x, currentCursorPos.y + 4.0f});
        cursorShape.setFillColor(sf::Color(100, 200, 255, 220));
        window.draw(cursorShape);

        window.display();
    }
};

int main() {
    GameEditor editor;
    editor.run();
    return 0;
}