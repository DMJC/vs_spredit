#include <filesystem> // C++17, but widely used with C++20
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <map>
#include <memory>
#include "image_item.h"

namespace fs = std::filesystem;

struct ParsedFrameInfo {
    fs::path image_path;
    bool has_alpha_mask_in_frame_line = false; // For .ani files, if 'true' is present

    // Cropping parameters (if found for this frame or overall)
    double mins = 0.0; // Default to full image (0%)
    double mint = 0.0;
    double maxs = 1.0; // Default to full image (100%)
    double maxt = 1.0;

    bool has_cropping = false; // True if mins/mint/maxs/maxt were explicitly defined
};

// --- Helper Functions ---
// Checks for .ani extension
bool ends_with(const std::string& str, const std::string& suffix) {
    if (str.length() < suffix.length()) {
        return false;
    }
    return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
}



// Checks if a string consists only of digits (and optional sign)
bool is_integer(const std::string& s) {
    if (s.empty()) return false;
    size_t start = 0;
    if (s[0] == '-' || s[0] == '+') {
        start = 1;
        if (s.length() == 1) return false; // Just a sign, not a number
    }
    for (size_t i = start; i < s.length(); ++i) {
        if (!std::isdigit(s[i])) {
            return false;
        }
    }
    return true;
}

// Checks if a string can be interpreted as a double
bool is_double(const std::string& s) {
    if (s.empty()) return false;
    std::istringstream iss(s);
    double d;
    iss >> d;
    return iss.eof() && !iss.fail();
}

// Function to parse min/max cropping parameters from a string
// Returns true if all 4 parameters were successfully found and parsed.
bool parse_cropping_params(const std::string& line_part, double& mins, double& mint, double& maxs, double& maxt) {
    std::map<std::string, double*> param_map;
    param_map["mins"] = &mins;
    param_map["mint"] = &mint;
    param_map["maxs"] = &maxs;
    param_map["maxt"] = &maxt;

    int found_count = 0; // Track how many parameters we successfully parse

    std::stringstream ss(line_part);
    std::string segment;

    while (std::getline(ss, segment, ',')) {
        size_t eq_pos = segment.find('=');
        if (eq_pos != std::string::npos) {
            std::string key_str = segment.substr(0, eq_pos);
            std::string val_str = segment.substr(eq_pos + 1);

            // Trim whitespace from key and value
            key_str.erase(0, key_str.find_first_not_of(" \t\n\r\f\v"));
            key_str.erase(key_str.find_last_not_of(" \t\n\r\f\v") + 1);
            val_str.erase(0, val_str.find_first_not_of(" \t\n\r\f\v"));
            val_str.erase(val_str.find_last_not_of(" \t\n\r\f\v") + 1);

            if (param_map.count(key_str) && is_double(val_str)) {
                *param_map[key_str] = std::stod(val_str);
                found_count++;
            }
        }
    }
    return found_count == 4; // All 4 parameters must be found
}


// --- Main load_spr_file function ---
std::shared_ptr<ImageItem> load_spr_file(const std::string& filename, const fs::path& asset_root_dir) {
    auto item = std::make_shared<ImageItem>();
    fs::path spr_filepath(filename);

    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Error: Cannot open SPR file: " << filename << std::endl;
        return nullptr;
    }

    std::string line;
    bool initial_is_animation_determination = false;

    // Helper lambda for resolving paths relative to the asset root
    auto resolve_path = [&](const std::string& relative_path_str) -> fs::path {
        fs::path p(relative_path_str);
        if (p.is_absolute()) {
            return p; // If it's already absolute, use it directly
        }
        // If it's relative, combine with the asset_root_dir
        return asset_root_dir / p;
    };
    getline(file, line);
    std::istringstream iss_line1(line);
    std::string token1, token2;
    iss_line1 >> token1 >> token2;
    if (ends_with(token1, ".ani")) {
        item->is_animation = true;
        initial_is_animation_determination = true;
        std::cout << "File extension is .ani: Assuming animation." << std::endl;
    } else {
        if (is_double(token1) && is_double(token2)) {
            item->is_animation = true;
            initial_is_animation_determination = true;
            std::cout << "Line 1 has two doubles: Assuming animation." << std::endl;
        } 
        if (ends_with(token1,".ani")){
        
        }
        else {
            item->is_animation = false;
            initial_is_animation_determination = false;
            item->static_image_filepath = resolve_path(token1);
            if (token2 == "0") {
                item->has_static_alpha_mask = false;
            } else {
                item->has_static_alpha_mask = true;
                item->alpha_mask_filepath = resolve_path(token2);
            }
        }
    }
    // --- Line 2: Always two doubles, ignore content ---
    if (!initial_is_animation_determination) {
        if (std::getline(file, line)) { /* ignore */ }
        else {
            std::cerr << "Warning: SPR file " << filename << " ended prematurely before Line 2 (ignored doubles)." << std::endl;
            return nullptr;
        }
    }

    // --- Line 3: number_of_frames, time_on_each_frame_ms, and optional overall cropping ---
    int declared_num_frames = 0;
    double overall_mins = 0.0, overall_mint = 0.0, overall_maxs = 1.0, overall_maxt = 1.0;
    bool has_overall_cropping = false;
    if (std::getline(file, line)) {
        std::istringstream iss_line3(line);
        std::string token_num_frames_str, token_time_ms_str;
        iss_line3 >> token_num_frames_str >> token_time_ms_str;
        std::cout << " Frames: "<< token_num_frames_str << " Time: " << token_time_ms_str << std::endl;

        if (is_integer(token_num_frames_str) && is_integer(token_time_ms_str)) {
            declared_num_frames = std::stoi(token_num_frames_str);
            item->frame_delay_ms = std::stod(token_time_ms_str);
            std::cout << "Line 3: Declared Frames=" << declared_num_frames << ", Frame Delay=" << item->frame_delay_ms << std::endl;

            std::string remaining_line;
            if (std::getline(iss_line3, remaining_line)) {
                 if (parse_cropping_params(remaining_line, overall_mins, overall_mint, overall_maxs, overall_maxt)) {
                     has_overall_cropping = true;
                     std::cout << "Line 3: Found overall cropping parameters." << std::endl;
                 }
            }

            if (!item->is_animation && declared_num_frames == 0) {
                 std::cout << "Line 3: 0 0, confirming static sprite with Line 1 image." << std::endl;
            } else {
                item->is_animation = true;
            }
        } else {
            std::cerr << "Warning: SPR file " << filename << " Line 3 not in expected 'int int' format. " << line << "  This might indicate an issue." << std::endl;
            if (!item->is_animation) {
                std::cerr << "Error: SPR file " << filename << " is static but has malformed Line 3. Aborting." << std::endl;
                return nullptr;
            }
            declared_num_frames = 0;
            item->frame_delay_ms = 100;
        }
    } else {
        std::cerr << "Error: SPR file " << filename << " missing Line 3 (frame count/time)." << std::endl;
        return nullptr;
    }

    // --- Load Gdk::Pixbuf(s) ---
    std::vector<ParsedFrameInfo> parsed_frames_info;

    if (!item->is_animation) {
        try {
            auto static_pixbuf = Gdk::Pixbuf::create_from_file(item->static_image_filepath.string());
            item->frames.push_back(static_pixbuf);
        } catch (const Glib::Error& ex) {
            std::cerr << "Error loading static image file '" << item->static_image_filepath << "': " << ex.what() << std::endl;
            return nullptr;
        }
    } else {
        std::cout << "Reading animation frame paths..." << std::endl;
        while (std::getline(file, line)) {
            std::istringstream iss_frame(line);
            std::string image_path_str;
            iss_frame >> image_path_str;
            if (image_path_str.empty()) continue;

            ParsedFrameInfo current_frame_info;
            current_frame_info.image_path = resolve_path(image_path_str); // Use the new helper

            std::string remaining_frame_line;
            if (std::getline(iss_frame, remaining_frame_line)) {
                std::istringstream iss_remaining(remaining_frame_line);
                std::string true_keyword_check;
                iss_remaining >> true_keyword_check;
                true_keyword_check.erase(0, true_keyword_check.find_first_not_of(" \t\n\r\f\v")); true_keyword_check.erase(true_keyword_check.find_last_not_of(" \t\n\r\f\v") + 1);

                if (true_keyword_check == "true") {
                    current_frame_info.has_alpha_mask_in_frame_line = true;
                    if (std::getline(iss_remaining, remaining_frame_line)) {
                        if (parse_cropping_params(remaining_frame_line, current_frame_info.mins, current_frame_info.mint, current_frame_info.maxs, current_frame_info.maxt)) {
                            current_frame_info.has_cropping = true;
                        }
                    }
                } else {
                    remaining_frame_line = true_keyword_check + remaining_frame_line;
                    if (parse_cropping_params(remaining_frame_line, current_frame_info.mins, current_frame_info.mint, current_frame_info.maxs, current_frame_info.maxt)) {
                        current_frame_info.has_cropping = true;
                    }
                }
            }

            if (!current_frame_info.has_cropping && has_overall_cropping) {
                current_frame_info.mins = overall_mins;
                current_frame_info.mint = overall_mint;
                current_frame_info.maxs = overall_maxs;
                current_frame_info.maxt = overall_maxt;
                current_frame_info.has_cropping = true;
            }

            parsed_frames_info.push_back(current_frame_info);
        }

        if (declared_num_frames > 0 && parsed_frames_info.size() != static_cast<size_t>(declared_num_frames)) {
            std::cerr << "Warning: Mismatch between declared number of frames (" << declared_num_frames
                      << ") and actual frames read (" << parsed_frames_info.size() << ") in " << filename << std::endl;
        }

        for (const auto& p_frame_info : parsed_frames_info) {
            try {
                auto pixbuf = Gdk::Pixbuf::create_from_file(p_frame_info.image_path.string());

                if (p_frame_info.has_cropping && pixbuf) {
                    int src_x = static_cast<int>(pixbuf->get_width() * p_frame_info.mins);
                    int src_y = static_cast<int>(pixbuf->get_height() * p_frame_info.mint);
                    int width = static_cast<int>(pixbuf->get_width() * (p_frame_info.maxs - p_frame_info.mins));
                    int height = static_cast<int>(pixbuf->get_height() * (p_frame_info.maxt - p_frame_info.mint));

                    if (width > 0 && height > 0) {
                        pixbuf = pixbuf->create_subpixbuf(pixbuf, src_x, src_y, width, height);
                    } else {
                        std::cerr << "Warning: Invalid cropping dimensions for frame " << p_frame_info.image_path << ". Not cropping." << std::endl;
                    }
                }
                item->frames.push_back(pixbuf);
            } catch (const Glib::Error& ex) {
                std::cerr << "Error loading animation frame '" << p_frame_info.image_path << "': " << ex.what() << std::endl;
                item->frames.push_back({});
            }
        }
    }

    if (!item->is_animation && !item->frames.empty() && item->frames.size() > 1) {
        std::cerr << "Warning: SPR file " << filename << " identified as static, but multiple frames were loaded. Forcing to animation." << std::endl;
        item->is_animation = true;
    }

    return item;
}
