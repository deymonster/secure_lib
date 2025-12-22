#ifndef XORSTR_HPP
#define XORSTR_HPP

#include <string>
#include <vector>

/*
 * Простая реализация шифрования строк на этапе компиляции (XOR).
 * Строки хранятся в бинарнике в зашифрованном виде и расшифровываются
 * только при вызове макроса XOR() или XORS().
 */

namespace xorstr_impl {

    // Используем __TIME__ как часть ключа, чтобы каждая сборка имела уникальный ключ
    constexpr char time_seed = static_cast<char>(__TIME__[7] + __TIME__[6] * 10 + __TIME__[4] * 60);

    template <int N, char K>
    struct XorString {
        char _encrypted[N];

        constexpr XorString(const char(&str)[N]) : _encrypted{} {
            for (int i = 0; i < N; ++i) {
                // XOR с ключом K и индексом i для вариативности
                _encrypted[i] = str[i] ^ K ^ (i % 55); 
            }
        }
    };

    template <int N, char K>
    struct XorWrapper {
        const XorString<N, K> _str;

        constexpr XorWrapper(const char(&str)[N]) : _str(str) {}

        // Расшифровка в std::string
        std::string decrypt() const {
            std::string s;
            s.resize(N - 1); // N включает null-terminator
            for (int i = 0; i < N - 1; ++i) {
                s[i] = _str._encrypted[i] ^ K ^ (i % 55);
            }
            return s;
        }
    };
}

// Основные макросы для использования

// Возвращает std::string (безопасно, память управляется автоматически)
#define XORS(str) (xorstr_impl::XorWrapper<sizeof(str), (char)(sizeof(str) + xorstr_impl::time_seed)>(str).decrypt())

// Возвращает C-style string (const char*). 
// ВНИМАНИЕ: Результат временный, используйте сразу или копируйте.
#define XOR(str) (XORS(str).c_str())

#endif // XORSTR_HPP