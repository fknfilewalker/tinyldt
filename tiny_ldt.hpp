/*
MIT License

Copyright(c) 2021 - 2024 Lukas Lipp

Permission is hereby granted, free of charge, to any person obtaining a copy
of this softwareand associated documentation files(the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :

The above copyright noticeand this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#pragma once

#include <string>
#include <vector>
#include <array>
#include <fstream>
#include <sstream>
#include <limits>

template <typename T>
struct tiny_ldt {
    static_assert(std::is_floating_point<T>::value, "T must be floating point");
    // https://docs.agi32.com/PhotometricToolbox/Content/Open_Tool/eulumdat_file_format.htm
    // https://web.archive.org/web/20190717124121/http://www.helios32.com/Eulumdat.htm
    // https://de.wikipedia.org/wiki/EULUMDAT
    struct light {
        light() :
    		ltyp{},
            lsym{},
            mc{}, mc1{}, mc2{},
            dc{},
            ng{},
            dg{},
            height_luminaire{},
            length_luminaire{},
            width_luminaire{},
            length_luminous_area{},
            width_luminous_area{},
            height_luminous_area_c0{},
            height_luminous_area_c90{},
            height_luminous_area_c180{},
            height_luminous_area_c270{},
            dff{}, lorl{},
            conversion_factor{}, tilt_of_luminaire{},
            n{},
            dr{}
        {}

        std::string manufacturer;   /* Company identification/databank/version/format identification */
        uint32_t ltyp;              /* Type indicator (0 - point source with no symmetry; 1 - symmetry  about the vertical axis; 2 - linear luminaire; 3 - point source with any other symmetry. Note: only linear luminaires, Ityp = 2, are being subdivided in longitudinal and transverse directions) */
        uint32_t lsym;              /* Symmetry indicator (0 ... no symmetry; 1 - symmetry about the vertical axis; 2 - symmetry to plane C0-C180; 3 - symmetry to plane C90-C270; 4 - symmetry to plane C0-C180 and to plane C90-C270) */
        uint32_t mc, mc1, mc2;      /* Number of C-planes between 0 and 360 degrees (usually 24 for interior, 36 for road lighting luminaires) */
        T dc;                       /* Distance between C-planes (Dc = 0 for non-equidistantly available C-planes) */
        uint32_t ng;                /* Number of luminous intensities in each C-plane (usually 19 or 37) */
        T dg;                       /* Distance between luminous intensities per C-plane (Dg = 0 for non-equidistantly available luminous intensities in C-planes) */

        std::string measurement_report_number;
        std::string luminaire_name;
        std::string luminaire_number;
        std::string file_name;
        std::string date_user;

        uint32_t height_luminaire;          /* mm */
        uint32_t length_luminaire;          /* mm */
        uint32_t width_luminaire;           /* mm */

        uint32_t length_luminous_area;      /* mm */
        uint32_t width_luminous_area;       /* mm */
        uint32_t height_luminous_area_c0;   /* mm */
        uint32_t height_luminous_area_c90;  /* mm */
        uint32_t height_luminous_area_c180; /* mm */
        uint32_t height_luminous_area_c270; /* mm */

        T dff;                              /* % */
        T lorl;                             /* % */
        T conversion_factor;
        uint32_t tilt_of_luminaire;
        uint32_t n;

        struct lamp_data_s {
            lamp_data_s() :
                number_of_lamps{},
                total_luminous_flux{},
                color_temperature{},
                color_rendering_group{},
                watt{}
            {}

            int number_of_lamps; // negative sign indicates absolute photometry
            std::string type_of_lamps;
            uint32_t total_luminous_flux;   /* lm */
            uint32_t color_temperature;
            uint32_t color_rendering_group;
            T watt;                         /* W */
        };
        std::vector<lamp_data_s> lamp_data;

        std::array<T, 10> dr;
        std::vector<T> angles_c;
        std::vector<T> angles_g;
        std::vector<T> luminous_intensity_distribution; /* cd/1000 lumens */
    };

    static bool load_ldt(const std::string& filename, std::string& err_out, std::string& warn_out, light& ldt_out) {
        std::ifstream f(filename);
        if (!f) {
            err_out = "Failed reading file: " + filename;
            return false;
        }

        ldt_out = {};
        std::string line;

#define NEXT_LINE(name) if (!std::getline(f, line)) { err_out = "Error reading <" name "> property: " + filename; f.close(); return false; }
#define CATCH(a) try{a;} catch (...) { warn_out = "Some values could not be read"; }

        /* line  1 */ NEXT_LINE("Manufacturer") ldt_out.manufacturer = line;
        /* line  2 */ NEXT_LINE("Type") CATCH(convertToType(line, ldt_out.ltyp))
        /* line  3 */ NEXT_LINE("Symmetry") CATCH(convertToType(line, ldt_out.lsym))
        /* line  4 */ NEXT_LINE("Mc") CATCH(convertToType(line, ldt_out.mc))
        if (calc_mc1_mc2(ldt_out)) {
            err_out = "Error reading light symmetry";
            return false;
        }
        /* line  5 */ NEXT_LINE("Dc") CATCH(convertToType(line, ldt_out.dc))
        /* line  6 */ NEXT_LINE("Ng") CATCH(convertToType(line, ldt_out.ng))
        /* line  7 */ NEXT_LINE("Dg") CATCH(convertToType(line, ldt_out.dg))

        /* line  8 */ NEXT_LINE("Measurement report number") ldt_out.measurement_report_number = line;
        /* line  9 */ NEXT_LINE("Luminaire name") ldt_out.luminaire_name = line;
        /* line 10 */ NEXT_LINE("Luminaire number") ldt_out.luminaire_number = line;
        /* line 11 */ NEXT_LINE("File name") ldt_out.file_name = line;
        /* line 12 */ NEXT_LINE("Date/user") ldt_out.date_user = line;

        /* line 13 */ NEXT_LINE("Length/diameter of luminaire") CATCH(convertToType(line, ldt_out.length_luminaire))
		/* line 14 */ NEXT_LINE("Width of luminaire") CATCH(convertToType(line, ldt_out.width_luminaire))
		/* line 15 */ NEXT_LINE("Height of luminaire") CATCH(convertToType(line, ldt_out.height_luminaire))
		/* line 16 */ NEXT_LINE("Length/diameter of luminous area") CATCH(convertToType(line, ldt_out.length_luminous_area))
		/* line 17 */ NEXT_LINE("Width of luminous area") CATCH(convertToType(line, ldt_out.width_luminous_area))
		/* line 18 */ NEXT_LINE("Height of luminous area C0-plane") CATCH(convertToType(line, ldt_out.height_luminous_area_c0))
		/* line 19 */ NEXT_LINE("Height of luminous area C90-plane") CATCH(convertToType(line, ldt_out.height_luminous_area_c90))
		/* line 20 */ NEXT_LINE("Height of luminous area C180-plane") CATCH(convertToType(line, ldt_out.height_luminous_area_c180))
		/* line 21 */ NEXT_LINE("Height of luminous area C270-plane") CATCH(convertToType(line, ldt_out.height_luminous_area_c270))
		/* line 22 */ NEXT_LINE("Downward flux fraction") CATCH(convertToType(line, ldt_out.dff))
		/* line 23 */ NEXT_LINE("Light output ratio luminaire") CATCH(convertToType(line, ldt_out.lorl))
		/* line 24 */ NEXT_LINE("Conversion factor for luminous intensities") CATCH(convertToType(line, ldt_out.conversion_factor))
		/* line 25 */ NEXT_LINE("Tilt of luminaire during measurement") CATCH(convertToType(line, ldt_out.tilt_of_luminaire))
		/* line 26 */ NEXT_LINE("Number of standard sets of lamps") CATCH(convertToType(line, ldt_out.n))

        // for each in the file defined lamp
        ldt_out.lamp_data.resize(ldt_out.n);
        for (auto& ld : ldt_out.lamp_data) {
            /* line 26a */ NEXT_LINE("Number of lamps") CATCH(convertToType(line, ld.number_of_lamps))
        }
        for (auto& ld : ldt_out.lamp_data) {
            /* line 26b */ NEXT_LINE("Type of lamps") ld.type_of_lamps = line;
        }
        for (auto& ld : ldt_out.lamp_data) {
            /* line 26c */ NEXT_LINE("Total luminous flux") CATCH(convertToType(line, ld.total_luminous_flux))
        }
        for (auto& ld : ldt_out.lamp_data) {
            /* line 26d */ NEXT_LINE("Color appearance") CATCH(convertToType(line, ld.color_temperature))
        }
        for (auto& ld : ldt_out.lamp_data) {
            /* line 26e */ NEXT_LINE("Color rendering group") CATCH(convertToType(line, ld.color_rendering_group))
        }
        for (auto& ld : ldt_out.lamp_data) {
            /* line 26f */ NEXT_LINE("Wattage including ballast") CATCH(convertToType(line, ld.watt))
        }
        for (T& v : ldt_out.dr) {
            /* line 27 */ NEXT_LINE("Direct ratios for room indices k = 0.6 ... 5") CATCH(convertToType(line, v))
        }
        ldt_out.angles_c.resize(ldt_out.mc);
        for (T& v : ldt_out.angles_c) {
            /* line 28 */ NEXT_LINE("Angles C") CATCH(convertToType(line, v))
        }
        ldt_out.angles_g.resize(ldt_out.ng);
        for (T& v : ldt_out.angles_g) {
            /* line 29 */ NEXT_LINE("Angles G") CATCH(convertToType(line, v))
        }
        // 30 ((Mc2 - Mc1 + 1) * Ng)
        // if lsym 0 : mc1 = 1, mc2 = mc
        // if lsym 1 : mc1 = 1, mc2 = 1
        // if lsym 2 : mc1 = 1, mc2 = mc / 2 + 1
        // if lsym 3 : mc1 = 3 * mc / 4 + 1, mc2 = mc1 + mc / 2
        // if lsym 4 : mc1 = 1, mc2 = mc / 4 + 1
        ldt_out.luminous_intensity_distribution.resize((static_cast<size_t>(ldt_out.mc2) - static_cast<size_t>(ldt_out.mc1) + 1) * static_cast<size_t>(ldt_out.ng));
        for (T& v : ldt_out.luminous_intensity_distribution) {
            /* line 29 */ NEXT_LINE("Luminous intensity distribution") CATCH(convertToType(line, v))
        }

#undef NEXT_LINE
#undef CATCH
        f.close();
        return true;
    }

    static bool write_ldt(const std::string& filename, const light& ldt, const uint32_t precision = std::numeric_limits<T>::max_digits10) {
        std::stringstream ss;
        ss.precision(precision);

        /* line  1 */ ss << ldt.manufacturer << std::endl;
        /* line  2 */ ss << ldt.ltyp << std::endl;
        /* line  3 */ ss << ldt.lsym << std::endl;
        /* line  4 */ ss << ldt.mc << std::endl;
        /* line  5 */ ss << ldt.dc << std::endl;
        /* line  6 */ ss << ldt.ng << std::endl;
        /* line  7 */ ss << ldt.dg << std::endl;

        /* line  8 */ ss << ldt.measurement_report_number << std::endl;
        /* line  9 */ ss << ldt.luminaire_name << std::endl;
        /* line 10 */ ss << ldt.luminaire_number << std::endl;
        /* line 11 */ ss << ldt.file_name << std::endl;
        /* line 12 */ ss << ldt.date_user << std::endl;

        /* line 13 */ ss << ldt.length_luminaire << std::endl;
        /* line 14 */ ss << ldt.width_luminaire << std::endl;
        /* line 15 */ ss << ldt.height_luminaire << std::endl;
        /* line 16 */ ss << ldt.length_luminous_area << std::endl;
        /* line 17 */ ss << ldt.width_luminous_area << std::endl;
        /* line 18 */ ss << ldt.height_luminous_area_c0 << std::endl;
        /* line 19 */ ss << ldt.height_luminous_area_c90 << std::endl;
        /* line 20 */ ss << ldt.height_luminous_area_c180 << std::endl;
        /* line 21 */ ss << ldt.height_luminous_area_c270 << std::endl;
        /* line 22 */ ss << ldt.dff << std::endl;
        /* line 23 */ ss << ldt.lorl << std::endl;
        /* line 24 */ ss << ldt.conversion_factor << std::endl;
        /* line 25 */ ss << ldt.tilt_of_luminaire << std::endl;
        /* line 26 */ ss << ldt.n << std::endl;

        // for each in the file defined lamp
        for (const auto& ld : ldt.lamp_data) {
            /* line 26a */ ss << ld.number_of_lamps << std::endl;
        }
        for (const auto& ld : ldt.lamp_data) {
            /* line 26b */ ss << ld.type_of_lamps << std::endl;
        }
        for (const auto& ld : ldt.lamp_data) {
            /* line 26c */ ss << ld.total_luminous_flux << std::endl;
        }
        for (const auto& ld : ldt.lamp_data) {
            /* line 26d */ ss << ld.color_temperature << std::endl;
        }
        for (const auto& ld : ldt.lamp_data) {
            /* line 26e */ ss << ld.color_rendering_group << std::endl;
        }
        for (const auto& ld : ldt.lamp_data) {
            /* line 26f */ ss << ld.watt << std::endl;
        }
        for (const T& v : ldt.dr) {
            /* line 27 */ ss << v << std::endl;
        }
        for (const T& v : ldt.angles_c) {
            /* line 28 */ ss << v << std::endl;
        }
        for (const T& v : ldt.angles_g) {
            /* line 29 */ ss << v << std::endl;
        }
        for (const T& v : ldt.luminous_intensity_distribution) {
            /* line 30 */ ss << v << std::endl;
        }

        std::ofstream file(filename, std::ios::out | std::ios::trunc);
        if (!file.is_open()) return false;
        file << ss.rdbuf();
        file.close();
        return true;
    }

private:
    template <typename U>
    static void convertToType(const std::string& s, U& out)
    {
        if (std::is_same<T, float>::value) out = std::stof(s);
        if (std::is_same<T, double>::value) out = std::stod(s);
        if (std::is_same<T, int>::value) out = std::stoi(s);
        if (std::is_same<T, uint32_t>::value) out = std::stoul(s);
    }

    static bool calc_mc1_mc2(light& l) {
        switch (l.lsym) {
        case 0:
            l.mc1 = 1; l.mc2 = l.mc;
            return false;
        case 1:
            l.mc1 = 1; l.mc2 = 1;
            return false;
        case 2:
            l.mc1 = 1; l.mc2 = l.mc / 2 + 1;
            return false;
        case 3:
            l.mc1 = 3 * l.mc / 4 + 1; l.mc2 = l.mc1 + l.mc / 2;
            return false;
        case 4:
            l.mc1 = 1; l.mc2 = l.mc / 4 + 1;
            return false;
        default:
            return true;
        }
    }
};
