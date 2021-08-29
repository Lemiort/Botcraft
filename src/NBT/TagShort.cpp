#include "botcraft/NBT/TagShort.hpp"

namespace Botcraft
{
    TagShort::TagShort()
    {

    }

    TagShort::~TagShort()
    {

    }

    const short TagShort::GetValue() const
    {
        return value;
    }

    void TagShort::SetValue(const short v)
    {
        value = v;
    }

    const TagType TagShort::GetType() const
    {
        return TagType::Short;
    }

    void TagShort::ReadImpl(ReadIterator &iterator, size_t &length)
    {
        value = ReadData<short>(iterator, length);
    }

    void TagShort::WriteImpl(WriteContainer &container) const
    {
        WriteData<short>(value, container);
    }

    const picojson::value TagShort::SerializeImpl() const
    {
        return picojson::value((double)value);
    }
}