/// @file thread_update_demo.cpp
/// @brief Demonstrates thread-safe UI updates from background threads using App::post()
///
/// This example shows how to update widgets from a background thread without
/// using timers. A simulated file download runs in a separate thread and
/// pushes UI updates to the main thread via app.post().

#include "cpptui.hpp"
#include <thread>
#include <chrono>
#include <atomic>

using namespace cpptui;

int main()
{
    App app;
    Theme::set_theme(Theme::Dark());

    // --- Widgets ---
    auto title = std::make_shared<Label>("Thread Update Demo");
    title->fixed_height = 1;
    title->fg_color = Theme::current().primary;

    auto status_label = std::make_shared<Label>("Status: Idle");
    status_label->fixed_height = 1;

    auto progress = std::make_shared<ProgressBar>();
    progress->fixed_height = 1;
    progress->show_text = true;
    progress->color = Theme::current().primary;

    auto log_area = std::make_shared<TextArea>();
    log_area->show_line_numbers = false;
    log_area->word_wrap = true;
    log_area->set_text("Ready. Click 'Start Download' to begin.\n");

    // --- Background task state ---
    std::atomic<bool> task_running{false};

    // --- Button to start the task ---
    auto start_button = std::make_shared<Button>("Start Download", nullptr);
    start_button->fixed_height = 1;
    start_button->bg_color = Theme::current().primary;

    auto footer = std::make_shared<ShortcutBar>();
    footer->add("q", "Quit");

    // --- Layout ---
    auto root = std::make_shared<Vertical>();
    auto border = std::make_shared<Border>(BorderStyle::Rounded);
    border->set_title(" Thread Update Demo ", Alignment::Center);
    border->add(root);

    root->add(title);
    root->add(std::make_shared<VerticalSpacer>(1));
    root->add(status_label);
    root->add(progress);
    root->add(std::make_shared<VerticalSpacer>(1));
    root->add(start_button);
    root->add(std::make_shared<VerticalSpacer>(1));
    root->add(log_area);
    root->add(footer);

    // --- Key handler: 'q' to quit ---
    app.register_exit_key('q');

    // Use a timer-free approach: post() from a background thread
    auto start_task = [&]()
    {
        if (task_running.load())
            return;
        task_running.store(true);

        std::thread worker([&]()
        {
            // Update button state from thread
            app.post([&]() {
                start_button->set_label("Downloading...");
                start_button->focusable = false; // Disable during task
            });

            // Simulated files to download
            std::vector<std::string> files = {
                "package-v2.3.1.tar.gz",
                "runtime-libs-x64.deb",
                "assets-hd.zip",
                "documentation.pdf",
                "config-templates.tar"
            };

            app.post([&]() {
                status_label->set_text("Status: Starting download...");
                log_area->set_text("Download started.\n");
                progress->value = 0.0f;
            });

            for (size_t i = 0; i < files.size(); ++i)
            {
                const auto& file = files[i];

                app.post([&, file, i]() {
                    std::string msg = "Downloading: " + file + "\n";
                    log_area->set_text(log_area->get_text() + msg);
                    status_label->set_text("Status: Downloading " + file);
                });

                // Simulate download with incremental progress
                int steps = 20;
                for (int step = 0; step < steps; ++step)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));

                    float file_progress = (float)(step + 1) / steps;
                    float overall = ((float)i + file_progress) / files.size();

                    app.post([&, overall]() {
                        progress->value = overall;
                    });
                }

                app.post([&, file]() {
                    log_area->set_text(log_area->get_text() + "  ✓ " + file + " complete\n");
                });
            }

            // Done
            app.post([&]() {
                progress->value = 1.0f;
                progress->color = Theme::current().success;
                status_label->set_text("Status: All downloads complete!");
                log_area->set_text(log_area->get_text() + "\nAll files downloaded successfully.\n");
                start_button->set_label("Download Complete");
                task_running.store(false);
            });
        });
        worker.detach();
    };

    start_button->set_on_click(start_task);

    app.run(border);
    return 0;
}
