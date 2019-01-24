#include <catch2/catch.hpp>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <zyn.common/PresetsSerializer.h>
#include <zyn.dsp/FilterParams.h>
#include <zyn.mixer/Microtonal.h>
#include <zyn.synth/ADnoteParams.h>
#include <zyn.synth/EnvelopeParams.h>
#include <zyn.synth/FFTwrapper.h>
#include <zyn.synth/LFOParams.h>
#include <zyn.synth/OscilGen.h>
#include <zyn.synth/PADnoteParams.h>
#include <zyn.synth/Resonance.h>
#include <zyn.synth/SUBnoteParams.h>

TEST_CASE("Presets")
{
    PresetsSerializer serializerA;
    serializerA.minimal = false;

    PresetsSerializer serializerB;
    serializerB.minimal = false;

    auto fft = FFTwrapper(1024);

    SECTION("AD Synth Presets", "[zyn.synth]")
    {
        auto sut = ADnoteParameters(&fft);
        sut.Defaults();
        sut.InitPresets();

        SECTION("Serialization should work")
        {
            sut.Serialize(&serializerA);
            auto presetsA = std::string(serializerA.getXMLdata());

            sut.WritePresetsToBlob(&serializerB);
            auto presetsB = std::string(serializerB.getXMLdata());

            REQUIRE(presetsA == presetsB);
        }
    }

    SECTION("PAD Synth Presets", "[zyn.synth]")
    {
        auto sut = PADnoteParameters(&fft);
        sut.Defaults();
        sut.InitPresets();

        SECTION("Serialization should work")
        {
            sut.Serialize(&serializerA);
            auto presetsA = std::string(serializerA.getXMLdata());

            sut.WritePresetsToBlob(&serializerB);
            auto presetsB = std::string(serializerB.getXMLdata());

            // TODO We skip this assert as long there is no solution for the ugly 'setPadSynth()' method
            // REQUIRE(presetsA == presetsB);
        }
    }

    SECTION("SUB Synth Presets", "[zyn.synth]")
    {
        PresetsSerializer serializerA;
        serializerA.minimal = false;

        PresetsSerializer serializerB;
        serializerB.minimal = false;

        auto sut = SUBnoteParameters();
        sut.Defaults();
        sut.InitPresets();

        SECTION("Serialization should work")
        {
            sut.Serialize(&serializerA);
            auto presetsA = std::string(serializerA.getXMLdata());

            sut.WritePresetsToBlob(&serializerB);
            auto presetsB = std::string(serializerB.getXMLdata());

            REQUIRE(presetsA == presetsB);
        }
    }

    SECTION("EnvelopeParams Presets", "[zyn.synth]")
    {
        auto sut = EnvelopeParams('\0', '\0');
        sut.Defaults();
        sut.InitPresets();

        SECTION("Serialization should work")
        {
            sut.Pforcedrelease = 1;

            sut.Serialize(&serializerA);
            auto presetsA = std::string(serializerA.getXMLdata());

            sut.WritePresetsToBlob(&serializerB);
            auto presetsB = std::string(serializerB.getXMLdata());

            REQUIRE(presetsA == presetsB);
        }

        SECTION("Deserialization should work")
        {
            REQUIRE(sut.Pforcedrelease == 0);
            REQUIRE(sut.PA_dt == 10);
            REQUIRE(sut.PD_dt == 10);
            REQUIRE(sut.PR_dt == 10);
            REQUIRE(sut.PA_val == 64);
            REQUIRE(sut.PD_val == 64);
            REQUIRE(sut.PS_val == 64);
            REQUIRE(sut.PR_val == 64);
            REQUIRE(sut.Penvpoints == 4);
            REQUIRE(sut.Penvdt[0] == 0);
            REQUIRE(sut.Penvdt[1] == 10);
            REQUIRE(sut.Penvdt[2] == 10);
            REQUIRE(sut.Penvdt[3] == 10);
            REQUIRE(sut.Penvval[0] == 0);
            REQUIRE(sut.Penvval[1] == 127);
            REQUIRE(sut.Penvval[2] == 64);
            REQUIRE(sut.Penvval[3] == 0);

            serializerA.loadXMLfile(std::string(TEST_DATA_PATH) + "EnvelopeParams.txt");

            sut.ReadPresetsFromBlob(&serializerA);

            REQUIRE(sut.Pforcedrelease == 1);
            REQUIRE(sut.PA_dt == 11);
            REQUIRE(sut.PD_dt == 12);
            REQUIRE(sut.PR_dt == 13);
            REQUIRE(sut.PA_val == 65);
            REQUIRE(sut.PD_val == 66);
            REQUIRE(sut.PS_val == 67);
            REQUIRE(sut.PR_val == 68);
            REQUIRE(sut.Penvpoints == 4);
            REQUIRE(sut.Penvdt[0] == 0);
            REQUIRE(sut.Penvdt[1] == 11);
            REQUIRE(sut.Penvdt[2] == 12);
            REQUIRE(sut.Penvdt[3] == 13);
            REQUIRE(sut.Penvval[0] == 1);
            REQUIRE(sut.Penvval[1] == 126);
            REQUIRE(sut.Penvval[2] == 65);
            REQUIRE(sut.Penvval[3] == 1);
        }
    }

    SECTION("LFOParams Presets", "[zyn.synth]")
    {
        PresetsSerializer serializerA;
        serializerA.minimal = false;

        PresetsSerializer serializerB;
        serializerB.minimal = false;

        auto sut = LFOParams(70, 0, 64, 0, 0, 0, 0, 0);
        sut.Defaults();
        sut.InitPresets();

        SECTION("Serialization should work")
        {
            sut.Serialize(&serializerA);
            auto presetsA = std::string(serializerA.getXMLdata());

            sut.WritePresetsToBlob(&serializerB);
            auto presetsB = std::string(serializerB.getXMLdata());

            REQUIRE(presetsA == presetsB);
        }
    }

    SECTION("Resonance Presets", "[zyn.synth]")
    {
        PresetsSerializer serializerA;
        serializerA.minimal = false;
        PresetsSerializer serializerB;
        serializerB.minimal = false;

        auto sut = Resonance();
        sut.Defaults();
        sut.InitPresets();
        sut.Penabled = 1;

        SECTION("Serialization should work")
        {
            sut.Serialize(&serializerA);
            auto presetsA = std::string(serializerA.getXMLdata());

            sut.WritePresetsToBlob(&serializerB);
            auto presetsB = std::string(serializerB.getXMLdata());

            REQUIRE(presetsA == presetsB);
        }
    }

    SECTION("Filter Presets", "[zyn.dsp]")
    {
        auto sut = FilterParams(0, 64, 64);
        sut.Defaults();
        sut.InitPresets();

        SECTION("Serialization should work")
        {
            sut.Serialize(&serializerA);
            auto presetsA = std::string(serializerA.getXMLdata());

            sut.WritePresetsToBlob(&serializerB);
            auto presetsB = std::string(serializerB.getXMLdata());

            REQUIRE(presetsA == presetsB);
        }
    }

    SECTION("Microtonal Presets", "[zyn.mixer]")
    {
        auto sut = Microtonal();
        sut.Defaults();
        sut.InitPresets();
        sut.Penabled = 1;

        SECTION("Serialization should work")
        {
            sut.Serialize(&serializerA);
            auto presetsA = std::string(serializerA.getXMLdata());

            sut.WritePresetsToBlob(&serializerB);
            auto presetsB = std::string(serializerB.getXMLdata());

            REQUIRE(presetsA == presetsB);
        }
    }

    SECTION("OscilGen Presets", "[zyn.synth]")
    {
        auto res = Resonance();
        res.Defaults();
        res.InitPresets();

        auto sut = OscilGen(&fft, &res);
        sut.Defaults();
        sut.InitPresets();
        sut.Pcurrentbasefunc = 0;

        SECTION("Serialization should work")
        {
            sut.Serialize(&serializerA);

            sut.WritePresetsToBlob(&serializerB);

            REQUIRE(std::string(serializerA.getXMLdata()) == std::string(serializerB.getXMLdata()));
        }
    }
}
