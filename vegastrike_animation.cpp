#include <gtkmm.h>
#include <cairomm/context.h>
#include <vector>
#include <memory>
#include <iostream>
#include <fstream>
#include "image_item.h"
#include "spr_parser.h"

class DrawingArea : public Gtk::DrawingArea {
public:
    std::vector<std::shared_ptr<ImageItem>> images;
    std::shared_ptr<ImageItem> selected_image;
    sigc::signal<void()> signal_image_selected;
    sigc::signal<void()> signal_delete_pressed;

    double drag_offset_x = 0;
    double drag_offset_y = 0;
    bool dragging = false;

    size_t global_frame_index = 0;  // Shared animation frame counter

    DrawingArea() {
        add_events(Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK | Gdk::POINTER_MOTION_MASK);
    }

    void set_frame_index(size_t index) {
        global_frame_index = index;
        queue_draw();
    }

    bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) override {
        double width = get_allocation().get_width();
        double height = get_allocation().get_height();
        cr->translate(width / 2.0, height / 2.0);  // move origin to center

        for (const auto& img : images) {
            auto pixbuf = img->get_frame(global_frame_index);
            if (!pixbuf) continue;

            double w = pixbuf->get_width() * img->scale_x;
            double h = pixbuf->get_height() * img->scale_y;
            auto scaled = pixbuf->scale_simple(w, h, Gdk::INTERP_BILINEAR);
            Gdk::Cairo::set_source_pixbuf(cr, scaled, img->x - w / 2.0, img->y - h / 2.0);
            cr->paint();

            if (img->selected) {
                cr->set_source_rgb(1.0, 0.0, 0.0);
                cr->set_line_width(2);
                cr->rectangle(img->x - w / 2.0, img->y - h / 2.0, w, h);
                cr->stroke();
            }
        }
        return true;
    }

    bool on_key_press_event(GdkEventKey* event) override {
        if (event->keyval == GDK_KEY_Delete) {
            std::cout << "Delete key pressed." << std::endl;
            signal_delete_pressed.emit();
            return true;
        }
        return Gtk::DrawingArea::on_key_press_event(event);
    }

    bool on_button_press_event(GdkEventButton* event) override {
        double cx = get_allocation().get_width() / 2.0;
        double cy = get_allocation().get_height() / 2.0;
        double ex = event->x - cx;
        double ey = event->y - cy;

        for (auto& img : images) {
            img->selected = false;
        }
        for (auto it = images.rbegin(); it != images.rend(); ++it) {
            if ((*it)->contains(ex, ey, global_frame_index)) {
                selected_image = *it;
                selected_image->selected = true;
                drag_offset_x = ex - selected_image->x;
                drag_offset_y = ey - selected_image->y;
                dragging = true;
                signal_image_selected.emit();
                break;
            }
        }
        queue_draw();
        return true;
    }

    bool on_button_release_event(GdkEventButton* event) override {
        dragging = false;
        return true;
    }

    bool on_motion_notify_event(GdkEventMotion* event) override {
        if (dragging && selected_image) {
            double cx = get_allocation().get_width() / 2.0;
            double cy = get_allocation().get_height() / 2.0;
            double ex = event->x - cx;
            double ey = event->y - cy;
            selected_image->x = ex - drag_offset_x;
            selected_image->y = ey - drag_offset_y;
            signal_image_selected.emit();
            queue_draw();
        }
        return true;
    }
};

class MainWindow : public Gtk::Window {
    Gtk::Box vbox{Gtk::ORIENTATION_VERTICAL};
    Gtk::Paned vpaned{Gtk::ORIENTATION_HORIZONTAL};
    Gtk::Paned hpaned{Gtk::ORIENTATION_VERTICAL};
    DrawingArea drawing_area;
    Gtk::Button add_png_button{"Add PNG"};
    Gtk::Button add_mask_button{"Add Mask"};
    Gtk::Button add_sprite_button{"Add Sprite"};
    Gtk::Button delete_element_button{"Delete PNG/Sprite"};
    Gtk::Button bring_front_button{"Bring to Front"};
    Gtk::Button send_back_button{"Send to Back"};
    Gtk::Box buttons{Gtk::ORIENTATION_VERTICAL};
    Gtk::Box controls{Gtk::ORIENTATION_VERTICAL};
    Gtk::Entry x_entry, y_entry, xscale_entry, yscale_entry;
    Gtk::Label x_label{"X:"}, y_label{"Y:"}, xscale_label{"X Scale:"}, yscale_label{"Y Scale:"};
    size_t animation_frame_index = 0;
    sigc::connection animation_timer;
protected:
    fs::path m_asset_root_dir;
public:
    MainWindow(const fs::path& asset_root_dir)
        : m_asset_root_dir(asset_root_dir) {

        set_title("Vegastrike Base/Sprite/Cockpit Editor");
        set_default_size(800, 600);
	    drawing_area.set_size_request(640, 480);
	    drawing_area.signal_delete_pressed.connect(sigc::mem_fun(*this, &MainWindow::on_del_ele_clicked));
        add(vbox);

        auto refActionGroup = Gio::SimpleActionGroup::create();
        refActionGroup->add_action("open", sigc::mem_fun(*this, &MainWindow::on_menu_file_open));
        refActionGroup->add_action("save sprite", sigc::mem_fun(*this, &MainWindow::on_menu_file_save_spr));
        refActionGroup->add_action("save cockpit", sigc::mem_fun(*this, &MainWindow::on_menu_file_save_cpt));
        refActionGroup->add_action("save base", sigc::mem_fun(*this, &MainWindow::on_menu_file_save_base));
        refActionGroup->add_action("quit", sigc::mem_fun(*this, &MainWindow::on_menu_file_quit));
        insert_action_group("example", refActionGroup);

        auto builder = Gtk::Builder::create();
        auto menu_bar = Gtk::manage(new Gtk::MenuBar());
        auto file_menu = Gtk::manage(new Gtk::Menu());
        auto item_file = Gtk::manage(new Gtk::MenuItem("_File", true));
        item_file->set_submenu(*file_menu);
        auto item_open = Gtk::manage(new Gtk::MenuItem("_Open", true));
        item_open->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_menu_file_open));
        file_menu->append(*item_open);
        auto item_save_spr = Gtk::manage(new Gtk::MenuItem("_Save Sprite", true));
        item_save_spr->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_menu_file_save_spr));
        file_menu->append(*item_save_spr);
        auto item_save_cpt = Gtk::manage(new Gtk::MenuItem("Save _Cockpit", true));
        item_save_cpt->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_menu_file_save_cpt));
        file_menu->append(*item_save_cpt);
        auto item_save_base = Gtk::manage(new Gtk::MenuItem("Save _Base", true));
        item_save_base->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_menu_file_save_base));
        file_menu->append(*item_save_base);
        auto item_quit = Gtk::manage(new Gtk::MenuItem("_Quit", true));
        item_quit->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_menu_file_quit));
        file_menu->append(*item_quit);
        menu_bar->append(*item_file);
        vbox.pack_start(*menu_bar, Gtk::PACK_SHRINK);
        vbox.pack_start(vpaned);
        vpaned.add1(drawing_area);
        vpaned.add2(hpaned);
        hpaned.add1(buttons);
        hpaned.add2(controls);
        buttons.pack_start(add_png_button, Gtk::PACK_SHRINK);
        buttons.pack_start(add_mask_button, Gtk::PACK_SHRINK);
    	buttons.pack_start(add_sprite_button, Gtk::PACK_SHRINK);
    	buttons.pack_start(delete_element_button, Gtk::PACK_SHRINK);
        buttons.pack_start(bring_front_button, Gtk::PACK_SHRINK);
        buttons.pack_start(send_back_button, Gtk::PACK_SHRINK);
        controls.pack_start(x_label, Gtk::PACK_SHRINK);
        controls.pack_start(x_entry, Gtk::PACK_SHRINK);
        controls.pack_start(y_label, Gtk::PACK_SHRINK);
        controls.pack_start(y_entry, Gtk::PACK_SHRINK);
        controls.pack_start(xscale_label, Gtk::PACK_SHRINK);
        controls.pack_start(xscale_entry, Gtk::PACK_SHRINK);
        controls.pack_start(yscale_label, Gtk::PACK_SHRINK);
        controls.pack_start(yscale_entry, Gtk::PACK_SHRINK);

        add_png_button.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_add_png_clicked));
        add_mask_button.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_add_png_clicked));
        add_sprite_button.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_add_spr_clicked));
        delete_element_button.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_del_ele_clicked));

        x_entry.signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_input_changed));
        y_entry.signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_input_changed));
        xscale_entry.signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_input_changed));
        yscale_entry.signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_input_changed));
        drawing_area.signal_image_selected.connect(sigc::mem_fun(*this, &MainWindow::on_image_selected));
        
        bring_front_button.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_bring_to_front));
        send_back_button.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_send_to_back));
        
        show_all_children();
    }

    void on_menu_file_open() { /* TODO */ }
    void on_menu_file_save_spr() { /* TODO */ }
    void on_menu_file_save_cpt() { /* TODO */ }
    void on_menu_file_save_base() { /* TODO */ }
    void on_menu_file_quit() { hide(); }

    void start_animation_timer() {
        if (animation_timer.connected())
            return; // already running

        animation_timer = Glib::signal_timeout().connect([this]() {
            animation_frame_index++;
            drawing_area.set_frame_index(animation_frame_index);  // assumes this method already exists
            return true; // continue running
        }, 100); // 100ms = 10 FPS
    }

    void on_add_png_clicked() {
        Gtk::FileChooserDialog dialog(*this, "Open PNG", Gtk::FILE_CHOOSER_ACTION_OPEN);
        dialog.add_button("Cancel", Gtk::RESPONSE_CANCEL);
        dialog.add_button("Open", Gtk::RESPONSE_OK);

        auto filter = Gtk::FileFilter::create();
        filter->add_mime_type("image/png");
        dialog.add_filter(filter);
    
        if (dialog.run() == Gtk::RESPONSE_OK) {
        auto image = std::make_shared<ImageItem>();
            try {
            auto pixbuf = Gdk::Pixbuf::create_from_file(dialog.get_filename());
                image->frames.push_back(pixbuf);  // single-frame PNG
                drawing_area.images.push_back(image);
                drawing_area.queue_draw();
            } catch (const Glib::Error& ex) {
                std::cerr << "Failed to load PNG: " << ex.what() << std::endl;
            }
        }
    }
 
    void on_add_spr_clicked() {
        Gtk::FileChooserDialog dialog(*this, "Open SPR File", Gtk::FILE_CHOOSER_ACTION_OPEN);
        dialog.add_button("Cancel", Gtk::RESPONSE_CANCEL);
        dialog.add_button("Open", Gtk::RESPONSE_OK);

        auto filter = Gtk::FileFilter::create();
        filter->set_name("SPR Files");
        filter->add_pattern("*.ani");
        filter->add_pattern("*.spr");
        dialog.add_filter(filter);

        if (dialog.run() == Gtk::RESPONSE_OK) {
            auto image = load_spr_file(dialog.get_filename(), m_asset_root_dir);
            if (image && !image->frames.empty()) {
                drawing_area.images.push_back(image);
                drawing_area.queue_draw();
                start_animation_timer();  // if using a global animation timer
            } else {
                std::cerr << "No frames loaded from SPR file." << std::endl;
            }
        }
    }

    void on_del_ele_clicked() {
        auto img = drawing_area.selected_image;
        if (!img) return;
		try {
        drawing_area.images.erase(std::remove(drawing_area.images.begin(), drawing_area.images.end(), img), drawing_area.images.end());
        } catch (...) {
            // Ignore invalid input
        }
	}

    bool try_parse_entry(const Gtk::Entry& entry, double& out) {
        try {
            out = std::stod(entry.get_text().raw());
            return true;
        } catch (...) {
            return false;
        }
    }

    void show_input_warning() {
        Gtk::MessageDialog dialog(*this, "Invalid numeric input", false,
                                  Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK, true);
        dialog.set_secondary_text("Please enter valid numeric values.");
        dialog.run();
    }

    void on_input_changed() {
        auto img = drawing_area.selected_image;
        if (!img) return;

        double new_x, new_y, new_sx, new_sy;

        if (!try_parse_entry(x_entry, new_x) ||
            !try_parse_entry(y_entry, new_y) ||
            !try_parse_entry(xscale_entry, new_sx) ||
            !try_parse_entry(yscale_entry, new_sy)) {
            show_input_warning();
            return;
        }

        if (new_sx == 0)
            new_sx = 0.1;
        if (new_sy == 0)
            new_sy = 0.1;

        img->x = new_x;
        img->y = new_y;
        img->scale_x = new_sx;
        img->scale_y = new_sy;
        drawing_area.queue_draw();
    }

    void on_image_selected() {
        auto img = drawing_area.selected_image;
        if (!img) return;
        x_entry.set_text(std::to_string(img->x));
        y_entry.set_text(std::to_string(img->y));
        xscale_entry.set_text(std::to_string(img->scale_x));
        yscale_entry.set_text(std::to_string(img->scale_y));
    }

    void on_send_to_back() {
        auto img = drawing_area.selected_image;
        if (!img) return;
        drawing_area.images.erase(std::remove(drawing_area.images.begin(), drawing_area.images.end(), img), drawing_area.images.end());
        drawing_area.images.push_back(img);
        drawing_area.queue_draw();
	std::cout << "Sending image to back" << std::endl;
    }

    void on_bring_to_front() {
        auto img = drawing_area.selected_image;
        if (!img) return;
        drawing_area.images.erase(std::remove(drawing_area.images.begin(), drawing_area.images.end(), img), drawing_area.images.end());
        drawing_area.images.insert(drawing_area.images.begin(), img);
        drawing_area.queue_draw();
	std::cout << "Bringing image to front" << std::endl;
    }
};

int main(int argc, char* argv[]) {
    auto app = Gtk::Application::create(argc, argv, "org.example.imageeditor");
    fs::path ASSET_ROOT_DIR;
    const std::string CONFIG_FILE_NAME = "vs_anim.cfg";

    std::ifstream config_file(CONFIG_FILE_NAME);
    std::string root_dir_str;

    if (config_file.is_open()) {
        if (std::getline(config_file, root_dir_str)) {
            // Trim whitespace from the loaded string
            root_dir_str.erase(0, root_dir_str.find_first_not_of(" \t\n\r\f\v"));
            root_dir_str.erase(root_dir_str.find_last_not_of(" \t\n\r\f\v") + 1);

            if (!root_dir_str.empty()) {
                ASSET_ROOT_DIR = root_dir_str;
                std::cout << "Loaded ASSET_ROOT_DIR from " << CONFIG_FILE_NAME << ": " << ASSET_ROOT_DIR << std::endl;
            } else {
                std::cerr << "Error: " << CONFIG_FILE_NAME << " is empty or contains only whitespace. Using default ASSET_ROOT_DIR." << std::endl;
                ASSET_ROOT_DIR = "/home/james/Project/WCUniverse/"; // Default fallback
            }
        } else {
            std::cerr << "Error: Could not read from " << CONFIG_FILE_NAME << ". Using default ASSET_ROOT_DIR." << std::endl;
            ASSET_ROOT_DIR = "/home/james/Project/WCUniverse/"; // Default fallback
        }
        config_file.close();
    } else {
        std::cerr << "Error: Could not open " << CONFIG_FILE_NAME << ". Using default ASSET_ROOT_DIR." << std::endl;
        ASSET_ROOT_DIR = "/home/james/Project/WCUniverse/"; // Default fallback
    }

    // Ensure the ASSET_ROOT_DIR exists and is a directory (optional, but good for robust error checking)
    if (!fs::is_directory(ASSET_ROOT_DIR)) {
        std::cerr << "Warning: ASSET_ROOT_DIR '" << ASSET_ROOT_DIR << "' is not a valid directory. Paths might fail." << std::endl;
        // You might want to exit here if it's critical for your application
        // return 1;
    }
    MainWindow window(ASSET_ROOT_DIR);
    return app->run(window);
}
