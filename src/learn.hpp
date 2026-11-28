#pragma once

#include <set>
#include <string>
#include <vector>

#include "nana.hpp"

namespace kestral {

struct LessonTest {
    std::string description;
    std::vector<std::string> inputs;
    std::vector<std::string> cli_args;
    std::vector<std::string> expect_contains;
    std::vector<std::string> expect_not_contains;
    std::string expect_exact;
    bool has_exact = false;
    bool use_temp_dir = false;
};

struct Lesson {
    int number = 0;
    std::string id;
    std::string track;
    std::string title;
    std::string goal;
    std::string explanation;
    std::string example;
    std::string challenge;
    std::string starter;
    std::string solution;
    std::vector<std::string> hints;
    std::vector<LessonTest> tests;
};

struct TestResult {
    std::string description;
    bool passed = false;
    std::string detail;
};

struct LearnProgress {
    std::set<std::string> completed;
    std::vector<std::pair<std::string, std::string>> drafts;
    int current = 1;
};

const std::vector<Lesson>& kestral_lessons();
std::vector<TestResult> evaluate_lesson(const Lesson& lesson, const std::string& source);

std::string learn_progress_path();
LearnProgress load_learn_progress();
void save_learn_progress(const LearnProgress& p);
void reset_learn_progress();

std::string format_curriculum_list(const std::set<std::string>& completed);
std::string render_lesson_map(int selected, const std::set<std::string>& completed,
                              int rows, int cols, const std::string& bg_mode = "mica");
std::string render_learn_workspace(const Lesson& lesson, NanaBuffer& buf,
                                   const std::vector<TestResult>& results,
                                   int hint_idx,
                                   const std::set<std::string>& completed,
                                   int rows, int cols,
                                   const std::string& bg_mode = "mica");

int run_learn(int lesson_num = 0, bool list_only = false, bool reset = false,
              const std::string& bg_override = "", bool managed_tty = false,
              RawTtyGuard* parent_tty = nullptr);

} // namespace kestral
