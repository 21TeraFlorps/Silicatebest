#pragma once
#include <Geode/Geode.hpp>
#include <algorithm>
#include <cstdint>
#include <functional>
#include <glaze/json/read.hpp>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "keybind.hpp"

struct SLBindingInterface {
    const virtual std::string& getId() = 0;

    virtual void* getValue() = 0;
    virtual void* getPrevious() = 0;

    // identifies the T that getValue()/getPrevious() actually point at
    virtual keybind::TypeKey getTypeKey() = 0;

    virtual void notifyChange() = 0;
    virtual std::shared_ptr<KeybindControl> createKeybind(RawKeybind& kb) = 0;

    virtual ~SLBindingInterface() = default;
};

class SLBindingManager {
    std::unordered_map<std::string, std::shared_ptr<SLBindingInterface>>
        m_values;

    using KeybindVector = std::vector<std::shared_ptr<KeybindControl>>;
    std::unordered_map<KeybindControl::HashT, KeybindVector> m_keybinds;

    struct HeldKeybind {
        std::weak_ptr<KeybindControl> keybind;
        double elapsed = 0.0;
        double repeatElapsed = 0.0;
        double repeatFrequency = 0.0;
        bool repeating = false;
    };
    std::unordered_map<int, std::vector<HeldKeybind>> m_heldKeybinds;

    SLBindingManager() = default;

    bool m_needsNewKey = false;
    KeybindControl* m_currentlyEdited = nullptr;

    bool m_hasRead = false;

    bool applyKeybind(const std::shared_ptr<KeybindControl>& keybind,
                      bool pressed) {
        auto value = m_values.find(keybind->getTag());
        if (value == m_values.end()) return false;
        if (keybind->getTypeKey() != value->second->getTypeKey()) {
            if (pressed) {
                geode::log::error(
                    "Keybind for {} disagrees with the value's type, "
                    "ignoring it",
                    keybind->getTag());
            }
            return false;
        }

        bool triggered = keybind->applyValue(pressed, value->second->getValue(),
                                             value->second->getPrevious());
        if (triggered) value->second->notifyChange();

        return true;
    }

    void releaseHeldKey(int key) {
        auto held = m_heldKeybinds.find(key);
        if (held == m_heldKeybinds.end()) return;

        auto bindings = std::move(held->second);
        m_heldKeybinds.erase(held);
        for (const auto& item : bindings) {
            if (auto keybind = item.keybind.lock()) {
                this->applyKeybind(keybind, false);
            }
        }
    }

    void loadDefaults() {
        struct DefaultBind {
            int key;
            int modifiers;
            const char* tag;
        };

        static constexpr DefaultBind defaults[] = {
            {70, 0, "ui.visible"},             // F
            {86, 0, "updater.frame_advance"},  // V
            {66, 0, "updater.advance_back"},   // B
            {67, 0, "updater.advance_one"},    // C
            {84, 0, "trajectory.enabled"},     // T
            {18, keybind::Alt, "ui.visible"},  // Alt
        };

        for (const auto& d : defaults) {
            RawKeybind raw{d.key, d.modifiers, d.tag, KeybindType::Toggle,
                           true,  "1"};

            auto kb = this->makeKeybind(raw);
            if (!kb) {
                geode::log::error("No value registered for default keybind {}",
                                  d.tag);
                continue;
            }

            this->registerKeybind(kb);
        }
    }

   public:
    static SLBindingManager* get() {
        static SLBindingManager instance;
        return &instance;
    }

    void writeToFile(const std::filesystem::path& path) {
        KeybindVector vec;
        for (auto it = m_keybinds.begin(); it != m_keybinds.end(); ++it) {
            for (auto& kb : it->second) {
                vec.push_back(kb);
            }
        }

        std::string buf;
        (void)glz::write<glz::opts{.prettify = true}>(vec, buf);

        std::ofstream fd(path);
        fd << buf;
    }

    void readFromFile(const std::filesystem::path& path) {
        if (m_hasRead) return;
        m_hasRead = true;

        std::vector<RawKeybind> vec;
        auto ec = glz::read_file_json(
            vec, geode::utils::string::pathToString(path), std::string{});
        if (ec) {
            std::string helpful = glz::format_error(ec, std::string{});
            geode::log::error(
                "Failed to read keybinds: {}, assuming default keybinds",
                helpful);

            this->loadDefaults();
            return;
        } else {
            geode::log::info("Read keybinds successfully!");
        }

        for (auto& raw : vec) {
            auto kb = this->makeKeybind(raw);
            if (!kb) {
                geode::log::error(
                    "Failed to register keybind for tag {}. Does it exist?",
                    raw.m_valueTag);
                continue;
            }

            this->registerKeybind(kb);
        }
    }

    void startEdit(KeybindControl* k) {
        m_currentlyEdited = k;
        m_needsNewKey = true;
    }

    void stopEdit() {
        m_currentlyEdited = nullptr;
        m_needsNewKey = false;
    }

    bool wantsCapture() { return m_needsNewKey; }
    bool isEditing(KeybindControl* k) {
        return m_currentlyEdited == k && m_needsNewKey;
    }

    void takeInput(int keyCode, bool shift, bool ctrl, bool alt) {
        if (m_currentlyEdited == nullptr) {
            m_needsNewKey = false;
            return;
        }

        if (keybind::isModifierKey(keyCode)) return;

        if (keyCode == 0x1B) {  // escape
            this->stopEdit();
            return;
        }

        if (keyCode == 0x08 || keyCode == 0x2E) {  // backspace, delete
            keyCode = 0;
            shift = ctrl = alt = false;
        }

        const auto oldHash = m_currentlyEdited->getHash();
        auto& bucket = m_keybinds[oldHash];
        auto it = std::ranges::find_if(
            bucket, [this](auto& k) { return k.get() == m_currentlyEdited; });

        std::shared_ptr<KeybindControl> kb;
        if (it != bucket.end()) {
            kb = *it;
            bucket.erase(it);
        }

        if (bucket.empty()) m_keybinds.erase(oldHash);

        m_currentlyEdited->m_key = keyCode;
        m_currentlyEdited->m_modifiers =
            keybind::packModifiers(ctrl, shift, alt);

        if (kb) this->registerKeybind(kb);

        m_currentlyEdited = nullptr;
        m_needsNewKey = false;
    }

    SLBindingManager(SLBindingManager const&) = delete;
    void operator=(SLBindingManager const&) = delete;

    void registerValue(std::shared_ptr<SLBindingInterface> value) {
        m_values[value->getId()] = value;
    }

    bool hasValue(const std::string& tag) const {
        return m_values.contains(tag);
    }

    std::vector<std::string> getBindableValueTags() const {
        std::vector<std::string> tags;
        tags.reserve(m_values.size());

        for (const auto& tag : m_values | std::views::keys) {
            // haha
            if (tag.starts_with('_') ||
                tag.find("_______") != std::string::npos)
                continue;
            tags.push_back(tag);
        }

        std::ranges::sort(tags);
        return tags;
    }

    std::shared_ptr<KeybindControl> makeKeybind(RawKeybind& raw) {
        auto value = m_values.find(raw.m_valueTag);
        if (value == m_values.end()) return nullptr;

        auto keybind = value->second->createKeybind(raw);
        if (keybind) keybind->setActive(raw.m_active);
        return keybind;
    }

    KeybindControl* retagKeybind(KeybindControl* kb, const std::string& tag) {
        if (kb == nullptr || kb->getTag() == tag) return nullptr;

        RawKeybind raw{kb->m_key,     kb->m_modifiers, tag,
                       kb->getType(), kb->isActive(),  kb->toString()};

        auto replacement = this->makeKeybind(raw);
        if (!replacement) return nullptr;

        auto bucket = m_keybinds.find(kb->getHash());
        if (bucket == m_keybinds.end()) return nullptr;

        auto it = std::ranges::find_if(bucket->second,
                                       [kb](auto& k) { return k.get() == kb; });
        if (it == bucket->second.end()) return nullptr;

        this->cancelHeldKeybind(kb);

        // key and modifiers are unchanged, so the bucket is still the right one
        if (m_currentlyEdited == kb) m_currentlyEdited = replacement.get();
        *it = replacement;

        geode::log::info("Retagged keybind to {}", tag);
        return replacement.get();
    }

    const std::unordered_map<KeybindControl::HashT, KeybindVector>&
    getKeybinds() {
        return m_keybinds;
    }

    void registerKeybind(std::shared_ptr<KeybindControl> kb) {
        geode::log::info("Registering new keybind for {}", kb->getTag());
        if (m_keybinds.contains(kb->getHash())) {
            m_keybinds[kb->getHash()].push_back(kb);
        } else {
            m_keybinds[kb->getHash()] = {kb};
        }
    }

    bool removeKeybind(KeybindControl* kb) {
        if (kb == nullptr) return false;

        auto bucket = m_keybinds.find(kb->getHash());
        if (bucket == m_keybinds.end()) return false;

        if (m_currentlyEdited == kb) this->stopEdit();
        this->cancelHeldKeybind(kb);
        const auto oldSize = bucket->second.size();
        std::erase_if(bucket->second,
                      [kb](const auto& item) { return item.get() == kb; });
        const bool removed = bucket->second.size() != oldSize;
        if (bucket->second.empty()) m_keybinds.erase(bucket);
        return removed;
    }

    void processKeyEvent(int key, bool pressed, bool ctrl, bool shift,
                         bool alt) {
        if (!pressed) {
            this->releaseHeldKey(key);
            return;
        }

        if (m_heldKeybinds.contains(key)) return;

        KeybindControl::HashT hash =
            keybind::makeHash(key, keybind::packModifiers(ctrl, shift, alt));

        auto bucket = m_keybinds.find(hash);
        if (bucket == m_keybinds.end()) return;

        std::vector<HeldKeybind> held;
        held.reserve(bucket->second.size());
        for (const auto& kb : bucket->second) {
            if (!kb->isActive()) continue;
            if (this->applyKeybind(kb, true)) held.push_back({kb});
        }

        if (!held.empty()) m_heldKeybinds.emplace(key, std::move(held));
    }

    bool isKeyHeld(int key) const { return m_heldKeybinds.contains(key); }

    void updateHeldKeybinds(double dt, double repeatFrequency,
                            double holdDelayMs) {
        if (!std::isfinite(dt) || dt <= 0.0) return;

        if (!std::isfinite(repeatFrequency)) repeatFrequency = 30.0;
        repeatFrequency = std::clamp(repeatFrequency, 1.0, 1000.0);
        if (!std::isfinite(holdDelayMs)) holdDelayMs = 500.0;
        const double holdDelay = std::max(0.0, holdDelayMs) / 1000.0;
        const double interval = 1.0 / repeatFrequency;

        for (auto& items : m_heldKeybinds | std::views::values) {
            for (auto& item : items) {
                auto kb = item.keybind.lock();
                if (!kb || !kb->isActive() ||
                    kb->getType() == KeybindType::Hold) {
                    continue;
                }

                item.elapsed += dt;
                uint64_t repeatCount = 0;
                if (!item.repeating) {
                    if (item.elapsed < holdDelay) continue;

                    item.repeating = true;
                    item.repeatElapsed = item.elapsed - holdDelay;
                    item.repeatFrequency = repeatFrequency;
                    repeatCount = 1;
                } else {
                    if (item.repeatFrequency != repeatFrequency) {
                        item.repeatElapsed = 0.0;
                        item.repeatFrequency = repeatFrequency;
                    }
                    item.repeatElapsed += dt;
                }

                const auto intervalCount =
                    static_cast<uint64_t>(item.repeatElapsed / interval);
                item.repeatElapsed -=
                    static_cast<double>(intervalCount) * interval;
                repeatCount += intervalCount;

                for (uint64_t repeat = 0; repeat < repeatCount; ++repeat) {
                    this->applyKeybind(kb, true);
                }
            }
        }
    }

    void cancelHeldKeybind(KeybindControl* target) {
        for (auto held = m_heldKeybinds.begin();
             held != m_heldKeybinds.end();) {
            std::erase_if(held->second, [this, target](const auto& item) {
                auto kb = item.keybind.lock();
                if (!kb || kb.get() != target) return !kb;
                this->applyKeybind(kb, false);
                return true;
            });

            if (held->second.empty()) {
                held = m_heldKeybinds.erase(held);
            } else {
                ++held;
            }
        }
    }

    void releaseAllHeldKeybinds() {
        auto held = std::move(m_heldKeybinds);
        m_heldKeybinds.clear();
        for (const auto& items : held | std::views::values) {
            for (const auto& item : items) {
                if (auto kb = item.keybind.lock()) {
                    this->applyKeybind(kb, false);
                }
            }
        }
    }
};

template <typename T>
struct SLValuePtr;

template <typename T>
class SLValue : public KeybindContainer<T>, public SLBindingInterface {
   private:
    T* m_value;
    T m_previousValue;

    std::optional<std::function<void(T&)>> m_callback;

   public:
    explicit operator bool() const = delete;

    SLValue(std::string tag, T* value)
        : m_value(value), m_previousValue(*value) {
        this->m_tag = tag;
    }

    T& inner() { return *m_value; }
    const T& inner() const { return *m_value; }

    SLValue& operator=(T value) {
        *m_value = value;
        return *this;
    }

    T operator()() { return *m_value; }

    void handle(std::function<void(T&)> callback) { m_callback = callback; }

    const std::string& getId() override { return this->m_tag; }

    void notifyChange() override {
        if (m_callback.has_value()) {
            m_callback.value()(*m_value);
        }
    }

    void* getValue() override { return m_value; }

    void* getPrevious() override { return &m_previousValue; }

    keybind::TypeKey getTypeKey() override { return keybind::typeKey<T>(); }

    static SLValuePtr<T> create(std::string tag, T* value) {
        auto binding = SLValuePtr<T>::make(tag, value);
        SLBindingManager::get()->registerValue(binding.inner);
        return binding;
    }

    std::shared_ptr<KeybindControl> createKeybind(RawKeybind& kb) override {
        return SLKeybind<T>::createFromString(
            kb.m_valueTag, kb.m_key, kb.m_type, kb.m_value, kb.m_modifiers);
    }
};

template <typename T>
struct SLValuePtr {
   private:
    using Self = SLValuePtr<T>;
    std::shared_ptr<SLValue<T>> inner;
    SLValuePtr() = default;

   public:
    explicit operator bool() const = delete;

    template <typename... Args>
    static Self make(Args&&... args) {
        Self instance;
        instance.inner =
            std::make_shared<SLValue<T>>(std::forward<decltype(args)>(args)...);
        return instance;
    }

    SLValue<T>* operator->() { return inner.get(); }
    SLValue<T> const* operator->() const { return inner.get(); }

    friend class SLBindingManager;
    friend class SLValue<T>;
};
