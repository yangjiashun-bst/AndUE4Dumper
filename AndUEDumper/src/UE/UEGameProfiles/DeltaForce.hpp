#pragma once

#include "../UEGameProfile.hpp"
using namespace UEMemory;

class DeltaForceProfile : public IGameProfile
{
public:
    DeltaForceProfile() = default;

    bool ArchSupprted() const override
    {
        auto e_machine = GetUnrealEngineELF().header().e_machine;
        return e_machine == EM_AARCH64;
    }

    std::string GetAppName() const override
    {
        return "Delta Force";
    }

    std::vector<std::string> GetAppIDs() const override
    {
        return {"com.proxima.dfm"};
    }

    bool isUsingCasePreservingName() const override
    {
        return false;
    }

    bool IsUsingFNamePool() const override
    {
        return true;
    }

    bool isUsingOutlineNumberName() const override
    {
        return false;
    }

    uintptr_t GetGUObjectArrayPtr() const override
    {
        std::vector<std::pair<std::string, int>> idaPatterns = {
            {"91 E1 03 ? AA E0 03 08 AA E2 03 1F 2A", -7},
            {"B4 21 0C 40 B9 ? ? ? ? ? ? ? 91", 5},
            {"9F E5 00 ? 00 E3 FF ? 40 E3 ? ? A0 E1", -2},
            {"96 df 02 17 ? ? ? ? 54 ? ? ? ? ? ? ? 91 e1 03 13 aa", 9},
            {"f4 03 01 2a ? 00 00 34 ? ? ? ? ? ? ? ? ? ? 00 54 ? 00 00 14 ? ? ? ? ? ? ? 91", 0x18},
            {"69 3e 40 b9 1f 01 09 6b ? ? ? 54 e1 03 13 aa ? ? ? ? f4 4f ? a9 ? ? ? ? ? ? ? 91", 0x18},
        };

        PATTERN_MAP_TYPE map_type = isEmulator() ? PATTERN_MAP_TYPE::ANY_R : PATTERN_MAP_TYPE::ANY_X;

        for (const auto &it : idaPatterns)
        {
            std::string ida_pattern = it.first;
            const int step = it.second;

            uintptr_t adrl = Arm64::Decode_ADRP_ADD(findIdaPattern(map_type, ida_pattern, step));
            if (adrl != 0) return adrl;
        }

        return 0;
    }

    uintptr_t GetNamesPtr() const override
    {
        PATTERN_MAP_TYPE map_type = isEmulator() ? PATTERN_MAP_TYPE::ANY_R : PATTERN_MAP_TYPE::ANY_X;

        std::string ida_pattern = "91 ? 10 81 52 ? ? 21 8b";
        const int step = -7;

        return Arm64::Decode_ADRP_ADD(findIdaPattern(map_type, ida_pattern, step));
    }

    UE_Offsets *GetOffsets() const override
    {
        static UE_Offsets offsets = UE_DefaultOffsets::UE4_25_27(isUsingCasePreservingName());

        static bool once = false;
        if (!once)
        {
            once = true;

            offsets.FNamePool.BlocksBit = 18;
            offsets.FNamePool.BlocksOff -= sizeof(void *);

            offsets.TUObjectArray.NumElements = sizeof(int32_t);
            offsets.TUObjectArray.Objects = offsets.TUObjectArray.NumElements + (sizeof(int32_t) * 3);

            offsets.UObject.ClassPrivate = sizeof(void *);
            offsets.UObject.OuterPrivate = offsets.UObject.ClassPrivate + sizeof(void *);
            offsets.UObject.ObjectFlags = offsets.UObject.OuterPrivate + sizeof(void *);
            offsets.UObject.NamePrivate = offsets.UObject.ObjectFlags + sizeof(int32_t);
            offsets.UObject.InternalIndex = offsets.UObject.NamePrivate + offsets.FName.Size;

            offsets.UStruct.PropertiesSize = offsets.UField.Next + (sizeof(void *) * 2) + sizeof(int32_t);
            offsets.UStruct.SuperStruct = offsets.UStruct.PropertiesSize + sizeof(int32_t);
            offsets.UStruct.Children = offsets.UStruct.SuperStruct + (sizeof(void *) * 2);
            offsets.UStruct.ChildProperties = offsets.UStruct.Children + (sizeof(void *) * 3);

            offsets.UFunction.NumParams = offsets.UStruct.ChildProperties + ((sizeof(void *) + sizeof(int32_t) * 2) * 2) + (sizeof(void *) * 5);
            offsets.UFunction.ParamSize = offsets.UFunction.NumParams + sizeof(int16_t);
            offsets.UFunction.EFunctionFlags = offsets.UFunction.ParamSize + sizeof(int16_t) + sizeof(int32_t);
            offsets.UFunction.Func = offsets.UFunction.EFunctionFlags + (sizeof(int32_t) * 2) + (sizeof(void *) * 3);

            offsets.FField.FlagsPrivate = sizeof(void *);
            offsets.FField.Next = offsets.FField.FlagsPrivate + (sizeof(void *) * 2);
            offsets.FField.ClassPrivate = offsets.FField.Next + sizeof(void *);
            offsets.FField.NamePrivate = offsets.FField.ClassPrivate + sizeof(void *);

            offsets.FProperty.ArrayDim = offsets.FField.NamePrivate + GetPtrAlignedOf(offsets.FName.Size) + sizeof(void *);
            offsets.FProperty.ElementSize = offsets.FProperty.ArrayDim + sizeof(int32_t);
            offsets.FProperty.PropertyFlags = offsets.FProperty.ElementSize + sizeof(int32_t);
            offsets.FProperty.Offset_Internal = offsets.FProperty.PropertyFlags + sizeof(int64_t) + sizeof(int32_t);
            offsets.FProperty.Size = offsets.FProperty.Offset_Internal + (sizeof(int32_t) * 3) + (sizeof(void *) * 4);
        }

        return &offsets;
    }

    std::string GetNameEntryString(uint8_t *entry) const override
    {
        std::string name = IGameProfile::GetNameEntryString(entry);

        auto dec_ansi = [](char *str, uint32_t len)
        {
            if (!str || !*str || len == 0) return;

            uint32_t key = 0;
            switch (len % 9)
            {
            case 0u:
                key = ((len & 0x1F) + len);
                break;
            case 1u:
                key = ((len ^ 0xDF) + len);
                break;
            case 2u:
                key = ((len | 0xCF) + len);
                break;
            case 3u:
                key = (33 * len);
                break;
            case 4u:
                key = (len + (len >> 2));
                break;
            case 5u:
                key = (3 * len + 5);
                break;
            case 6u:
                key = (((4 * len) | 5) + len);
                break;
            case 7u:
                key = (((len >> 4) | 7) + len);
                break;
            case 8u:
                key = ((len ^ 0xC) + len);
                break;
            default:
                key = ((len ^ 0x40) + len);
                break;
            }

            for (uint32_t i = 0; i < len; i++)
            {
                str[i] = (key & 0x80) ^ ~str[i];
            }
        };

        dec_ansi(name.data(), uint32_t(name.length()));

        return name;
    }
};
