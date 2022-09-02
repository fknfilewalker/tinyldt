/*
MIT License

Copyright(c) 2021 - 2022 Lukas Lipp

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

template <typename T>
class tiny_ldt {
    static_assert(std::is_same<T, float>::value || std::is_same<T, double>::value, "T must be float or double");
public:
    // https://docs.agi32.com/PhotometricToolbox/Content/Open_Tool/eulumdat_file_format.htm
    // https://web.archive.org/web/20190717124121/http://www.helios32.com/Eulumdat.htm
    // https://de.wikipedia.org/wiki/EULUMDAT
    struct light {
        light() : ltyp(0),
            lsym(0),
            mc(0), mc1(0), mc2(0),
            dc(0),
            ng(0),
            dg(0),
            height_luminaire(0),
            length_luminaire(0),
            width_luminaire(0),
            length_luminous_area(0),
            width_luminous_area(0),
            height_luminous_area_c0(0),
            height_luminous_area_c90(0),
            height_luminous_area_c180(0),
            height_luminous_area_c270(0),
            dff(0), lorl(0),
            conversion_factor(0), tilt_of_luminaire(0),
            n(0),
            dr({})
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
            lamp_data_s() : number_of_lamps(0),
                total_luminous_flux(0),
                color_temperature(0),
                color_rendering_group(0),
                watt(0)
            {}

            int number_of_lamps;
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

    static bool load_ldt(const std::string filename, std::string& err_out, std::string& warn_out, light& ldt_out) {
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
        /* line  2 */ NEXT_LINE("Type") CATCH(ldt_out.ltyp = std::stoi(line))
        /* line  3 */ NEXT_LINE("Symmetry") CATCH(ldt_out.lsym = std::stoi(line))
        /* line  4 */ NEXT_LINE("Mc") CATCH(ldt_out.mc = std::stoi(line))
        if (calc_mc1_mc2(ldt_out)) {
            err_out = "Error reading light symmetry";
            return false;
        }
        /* line  5 */ NEXT_LINE("Dc") CATCH(ldt_out.dc = toTemplateType(line))
        /* line  6 */ NEXT_LINE("Ng") CATCH(ldt_out.ng = std::stoi(line))
        /* line  7 */ NEXT_LINE("Dg") CATCH(ldt_out.dg = toTemplateType(line))

        /* line  8 */ NEXT_LINE("Measurement report number") ldt_out.measurement_report_number = line;
        /* line  9 */ NEXT_LINE("Luminaire name") ldt_out.luminaire_name = line;
        /* line 10 */ NEXT_LINE("Luminaire number") ldt_out.luminaire_number = line;
        /* line 11 */ NEXT_LINE("File name") ldt_out.file_name = line;
        /* line 12 */ NEXT_LINE("Date/user") ldt_out.date_user = line;

        /* line 13 */ NEXT_LINE("Length/diameter of luminaire") CATCH(ldt_out.length_luminaire = std::stoi(line))
        /* line 14 */ NEXT_LINE("Width of luminaire") CATCH(ldt_out.width_luminaire = std::stoi(line))
        /* line 15 */ NEXT_LINE("Height of luminaire") CATCH(ldt_out.height_luminaire = std::stoi(line))
        /* line 16 */ NEXT_LINE("Length/diameter of luminous area") CATCH(ldt_out.length_luminous_area = std::stoi(line))
        /* line 17 */ NEXT_LINE("Width of luminous area") CATCH(ldt_out.width_luminous_area = std::stoi(line))
        /* line 18 */ NEXT_LINE("Height of luminous area C0-plane") CATCH(ldt_out.height_luminous_area_c0 = std::stoi(line))
        /* line 19 */ NEXT_LINE("Height of luminous area C90-plane") CATCH(ldt_out.height_luminous_area_c90 = std::stoi(line))
        /* line 20 */ NEXT_LINE("Height of luminous area C180-plane") CATCH(ldt_out.height_luminous_area_c180 = std::stoi(line))
        /* line 21 */ NEXT_LINE("Height of luminous area C270-plane") CATCH(ldt_out.height_luminous_area_c270 = std::stoi(line))
        /* line 22 */ NEXT_LINE("Downward flux fraction") CATCH(ldt_out.dff = toTemplateType(line))
        /* line 23 */ NEXT_LINE("Light output ratio luminaire") CATCH(ldt_out.lorl = toTemplateType(line))
        /* line 24 */ NEXT_LINE("Conversion factor for luminous intensities") CATCH(ldt_out.conversion_factor = toTemplateType(line))
        /* line 25 */ NEXT_LINE("Tilt of luminaire during measurement") CATCH(ldt_out.tilt_of_luminaire = std::stoi(line))
        /* line 26 */ NEXT_LINE("Number of standard sets of lamps") CATCH(ldt_out.n = std::stoi(line))

        // for each in the file defined lamp
        ldt_out.lamp_data.resize(ldt_out.n);
        for (light::lamp_data_s& ld : ldt_out.lamp_data) {
            /* line 26a */ NEXT_LINE("Number of lamps") CATCH(ld.number_of_lamps = std::stoi(line))
        }
        for (light::lamp_data_s& ld : ldt_out.lamp_data) {
            /* line 26b */ NEXT_LINE("Type of lamps") ld.type_of_lamps = line;
        }
        for (light::lamp_data_s& ld : ldt_out.lamp_data) {
            /* line 26c */ NEXT_LINE("Total luminous flux") CATCH(ld.total_luminous_flux = std::stoi(line))
        }
        for (light::lamp_data_s& ld : ldt_out.lamp_data) {
            /* line 26d */ NEXT_LINE("Color appearance") CATCH(ld.color_temperature = std::stoi(line))
        }
        for (light::lamp_data_s& ld : ldt_out.lamp_data) {
            /* line 26e */ NEXT_LINE("Color rendering group") CATCH(ld.color_rendering_group = std::stoi(line))
        }
        for (light::lamp_data_s& ld : ldt_out.lamp_data) {
            /* line 26f */ NEXT_LINE("Wattage including ballast") CATCH(ld.watt = toTemplateType(line))
        }
        for (T& v : ldt_out.dr) {
            /* line 27 */ NEXT_LINE("Direct ratios for room indices k = 0.6 ... 5") CATCH(v = toTemplateType(line))
        }
        ldt_out.angles_c.resize(ldt_out.mc);
        for (T& v : ldt_out.angles_c) {
            /* line 28 */ NEXT_LINE("Angles C") CATCH(v = toTemplateType(line))
        }
        ldt_out.angles_g.resize(ldt_out.ng);
        for (T& v : ldt_out.angles_g) {
            /* line 29 */ NEXT_LINE("Angles G") CATCH(v = toTemplateType(line))
        }
        // 30 ((Mc2 - Mc1 + 1) * Ng)
        // if lsym 0 : mc1 = 1, mc2 = mc
        // if lsym 1 : mc1 = 1, mc2 = 1
        // if lsym 2 : mc1 = 1, mc2 = mc / 2 + 1
        // if lsym 3 : mc1 = 3 * mc / 4 + 1, mc2 = mc1 + mc / 2
        // if lsym 4 : mc1 = 1, mc2 = mc / 4 + 1
        ldt_out.luminous_intensity_distribution.resize((static_cast<size_t>(ldt_out.mc2) - static_cast<size_t>(ldt_out.mc1) + 1) * static_cast<size_t>(ldt_out.ng));
        for (T& v : ldt_out.luminous_intensity_distribution) {
            /* line 29 */ NEXT_LINE("Luminous intensity distribution") CATCH(v = toTemplateType(line))
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
        for (const light::lamp_data_s& ld : ldt.lamp_data) {
            /* line 26a */ ss << ld.number_of_lamps << std::endl;
        }
        for (const light::lamp_data_s& ld : ldt.lamp_data) {
            /* line 26b */ ss << ld.type_of_lamps << std::endl;
        }
        for (const light::lamp_data_s& ld : ldt.lamp_data) {
            /* line 26c */ ss << ld.total_luminous_flux << std::endl;
        }
        for (const light::lamp_data_s& ld : ldt.lamp_data) {
            /* line 26d */ ss << ld.color_temperature << std::endl;
        }
        for (const light::lamp_data_s& ld : ldt.lamp_data) {
            /* line 26e */ ss << ld.color_rendering_group << std::endl;
        }
        for (const light::lamp_data_s& ld : ldt.lamp_data) {
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
    static T toTemplateType(const std::string& s)
    {
        if (std::is_same<T, float>::value) return std::stof(s);
        if (std::is_same<T, double>::value) return std::stod(s);
        return 0;
    }

    static bool calc_mc1_mc2(light &l) {
        switch (l.lsym) {
        case 0:
            l.mc1 = 1; l.mc2 = l.mc;
            return false;
        case 1:
            l.mc1 = 1; l.mc2 = 1;
            return false;
        case 2:
            l.mc1 = 1; l.mc2 = l.mc/2+1;
            return false;
        case 3:
            l.mc1 = 3*l.mc/4+1; l.mc2 = l.mc1 + l.mc/2;
            return false;
        case 4:
            l.mc1 = 1; l.mc2 = l.mc/4+1;
            return false;
        default:
            return true;
        }
    }
};