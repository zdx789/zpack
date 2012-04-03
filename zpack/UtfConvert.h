#ifndef __UTF_CONVERT_H__
#define __UTF_CONVERT_H__

namespace zp
{
size_t utf8toutf16(const char* u8, size_t size, wchar_t* u16, size_t outBufferSize);

size_t utf16toutf8(const wchar_t* u16, size_t size, char* u8, size_t outBufferSize);
}

#endif
