#ifndef IMAGE_ITEM_H
#define IMAGE_ITEM_H

#include <vector>
#include <iostream>
#include <glibmm/refptr.h>
#include <gdkmm/pixbuf.h>
#include <filesystem> // C++17, but widely used with C++20
namespace fs = std::filesystem;

struct ImageItem {
    std::vector<Glib::RefPtr<Gdk::Pixbuf>> frames;  // Multiple frames
    double x = 0, y = 0;
    double scale_x = 1.0, scale_y = 1.0;
    bool selected = false;
    double frame_delay_ms = 100; // Delay between frames in milliseconds

    // Information about the original SPR file (for internal use, not part of public API)
    fs::path static_image_filepath;   // For static images
    fs::path alpha_mask_filepath;     // For static images, can be empty
    bool is_animation = false;        // True if it's an animation
    bool has_static_alpha_mask = false; // True if static_image has an alpha mask file

    // Access current frame (e.g. from external animation controller)
    Glib::RefPtr<Gdk::Pixbuf> get_frame(size_t frame_index) const {
        if (frames.empty())
            return {};
        return frames[frame_index % frames.size()];
    }

    bool contains(double px, double py, size_t frame_index = 0) const {
        auto pixbuf = get_frame(frame_index);
        if (!pixbuf) return false;

        double w = pixbuf->get_width() * scale_x;
        double h = pixbuf->get_height() * scale_y;
        double left = x - w / 2.0;
        double top = y - h / 2.0;
        return px >= left && px <= left + w && py >= top && py <= top + h;
    }

    // For debugging/inspection
    void print_info() const {
        std::cout << "--- ImageItem Info ---" << std::endl;
        std::cout << "Is Animation: " << (is_animation ? "true" : "false") << std::endl;

        if (!is_animation) {
            std::cout << "Static Image File: " << static_image_filepath << std::endl;
            std::cout << "Has Static Alpha Mask: " << (has_static_alpha_mask ? "true" : "false") << std::endl;
            if (has_static_alpha_mask) {
                std::cout << "Alpha Mask File: " << alpha_mask_filepath << std::endl;
            }
        } else {
            std::cout << "Number of Frames (loaded): " << frames.size() << std::endl;
            std::cout << "Frame Delay (ms): " << frame_delay_ms << std::endl;

            // This part might be less useful without the original parsedFrameInfo
            // but we can still show which frames were loaded.
            for (size_t i = 0; i < frames.size(); ++i) {
                std::cout << "  Loaded Frame " << i + 1 << " (Pixbuf valid: " << (frames[i] ? "true" : "false") << ")" << std::endl;
                if (frames[i]) {
                    std::cout << "    Dimensions: " << frames[i]->get_width() << "x" << frames[i]->get_height() << std::endl;
                }
            }
        }
        std::cout << "Scale: " << scale_x << "x, " << scale_y << "y" << std::endl;
        std::cout << "Position: " << x << ", " << y << std::endl;
        std::cout << "Selected: " << (selected ? "true" : "false") << std::endl;
        std::cout << "----------------------" << std::endl;
    }
};
#endif // IMAGE_ITEM_H
