namespace zp
{

///////////////////////////////////////////////////////////////////////////////////////////////////
size_t utf8toutf16(const char* u8, size_t size, wchar_t* u16, size_t outBufferSize)
{
	size_t converted = 0;

	while (size > 0)
	{
		if ((*u8 & 0x80) == 0)
		{
			*(u16++) = *(u8++);
			--size;
			++converted;
		}
		else if ((*u8 & 0xe0) == 0xc0)
		{
			if (size < 2)
			{
				break;
			}
			*(u16++) = (*u8 & 0x1f) | ((*(u8+1) & 0x3f) << 5);
			u8 += 2;
			size -= 2;
			++converted;
		}
		else if ((*u8 & 0xf0) == 0xe0)
		{
			if (size < 3)
			{
				break;
			}
			*u16 = ((*u8 & 0x0f) << 12) | ((*(u8+1) & 0x3f) << 6) | (*(u8+2) & 0x3f);
			u8 += 3;
			++u16;
			size -= 3;
			++converted;
		}
		else
		{
			break;
		}
		if (converted == outBufferSize)
		{
			break;
		}
	}
	return converted;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
size_t utf16toutf8(const wchar_t* u16, size_t size, char* u8, size_t outBufferSize)
{
	size_t converted = 0;

	while (size > 0)
	{
		if (*u16 < 0x80)
		{
			//1 byte
			if (converted == outBufferSize)
			{
				break;
			}
			*(u8++) = static_cast<char>(*(u16++));
			--size;
			++converted;
		}
		else if (*u16 < 0x800)
		{
			//2 bytes
			if (converted + 2 > outBufferSize)
			{
				break;
			}
			*u8 = (*u16 >> 6) | 0xc0;
			*(u8+1) = (*u16 & 0x3f) | 0x80;
			u8 += 2;
			++u16;
			--size;
			converted += 2;
		}
		else
		{
			//3 bytes
			if (converted + 3 > outBufferSize)
			{
				break;
			}
			*u8 = (*u16 >> 12) | 0xe0;
			*(u8+1) = ((*u16 >> 6) & 0x3f) | 0x80;
			*(u8+2) = (*u16 & 0x3f) | 0x80;
			u8 += 3;
			++u16;
			--size;
			converted += 3;
		}
	}
	return converted;
}

}
