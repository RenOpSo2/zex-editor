#include "../src/dispwidth.h"
#include "../src/render_buffer.h"

#include <cassert>
#include <cstring>
#include <iostream>

int main()
{
    std::cout << "Running C++ bridge smoke test...\n";

    unsigned char utf8[] = {0xE4, 0xB8, 0xAD}; // 中
    size_t consumed = 0;
    assert(char_width(utf8, sizeof(utf8), 0, 4, &consumed) == 2);
    assert(consumed == sizeof(utf8));

    unsigned char tab[] = {'\t'};
    assert(char_width(tab, sizeof(tab), 2, 4, &consumed) == 2);
    assert(consumed == 1);

    RenderBuffer rb{};
    rb_init(&rb);
    assert(rb.len == 0);
    assert(rb.cap == 0);
    assert(rb.overflowed == 0);

    assert(rb_append_str(&rb, "hello") == 0);
    assert(rb.len == 5);
    assert(std::memcmp(rb.data, "hello", 5) == 0);

    assert(rb_append_char(&rb, '!') == 0);
    assert(rb.len == 6);
    assert(std::memcmp(rb.data, "hello!", 6) == 0);

    rb_clear(&rb);
    assert(rb.len == 0);
    assert(rb_append_str(&rb, nullptr) == 0);

    rb_deinit(&rb);
    std::cout << "C++ bridge smoke test passed\n";
    return 0;
}