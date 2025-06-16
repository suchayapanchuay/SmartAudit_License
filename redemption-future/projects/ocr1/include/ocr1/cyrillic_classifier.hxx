inline
std::string_view russian(unsigned width, unsigned height, unsigned count, Pixel pixel) /*NOLINT*/
{
# include "./ocr1/classifiers/russian.hxx"
  return unknown;
}

constexpr Font::Replacement const russian_replacement[] {
{"ь\x1", "ы"},
{"Ь\x2", "Ы"},
{"\x1", ""},
{"\x2", ""},
};

inline
std::string_view cyrillic_tahoma_8_bold(unsigned width, unsigned height, unsigned count, Pixel pixel) /*NOLINT*/
{
# include "./ocr1/classifiers/cyrillic_tahoma_8_bold.hxx"
  return unknown;
}

constexpr Font::Replacement const cyrillic_tahoma_8_bold_replacement[] {
{"ь\x1", "ы"},
{"Ь\x2", "Ы"},
{"\x1", ""},
{"\x2", ""},
};

