#include <argparse/argparse.hpp>
#include <fstream>
#include <thread>
#include <fmt/core.h>

#include "CLI.h"
#include "debugger/Native.h"
#include "debugger/Source/Parser.h"
#include "t86-parser/parser.h"
#include "t86/os.h"
#include "threads_messenger.h"

dbg::DebuggingInfo ParseDebugInfo(std::ifstream& elf) {
    dbg::Parser p(elf);
    return p.Parse();
}

SourceFile ParseSourceFile(std::ifstream& ifs) {
    std::stringstream buffer;
    buffer << ifs.rdbuf();
    return buffer.str();
}

int main(int argc, char* argv[]) {
    argparse::ArgumentParser args("debugger");
    args.add_argument("file")
        .help("Input file with T86 assembly to be debugged.");
    
    try {
        args.parse_args(argc, argv);
    } catch (const std::runtime_error& e) {
        fmt::print(stderr, "{}\n", e.what());
        std::cerr << args;
        return 1;
    }
    
    Cli cli(args.get<std::string>("file"));
    cli.Run();
#if 0
    if (args.is_subcommand_used("remote")) {
        int port = remote_command.get<int>("port");
        // Initialize connection to debuggee
        auto debuggee_process = Native::Initialize(port);
        // Set up the native manager
        Native native(std::move(debuggee_process));
        // Run CLI
        Source source;
        CliInstance cli(std::move(native), std::move(source));
        return cli.Run();
    } else if (args.is_subcommand_used("run-t86")) {
        auto reg_count = run_command.get<int>("register-count");
        auto float_reg_count = run_command.get<int>("float-register-count");

        ThreadQueue<std::string> q1;
        ThreadQueue<std::string> q2;
        auto tm1 = std::make_unique<ThreadMessenger>(q1, q2);
        auto tm2 = std::make_unique<ThreadMessenger>(q2, q1);

        std::string fname = run_command.get<std::string>("file");
        std::ifstream file(fname);
        if (!file) {
            fmt::print(stderr, "Unable to open file '{}'\n", fname);
            exit(1);
        }

        Parser parser(file);
        auto program = parser.Parse();

        // Parse debug info
        Source source;
        file.clear();
        file.seekg(0);
        if (file) {
            auto debug_info = ParseDebugInfo(file);
            if (debug_info.line_mapping) {
                log_info("Found line mapping in debug info");
                source.RegisterLineMapping(std::move(*debug_info.line_mapping));
            }
        }
        // Parse source code
        if (auto path = run_command.present("--source")) {
            std::ifstream f(*path);
            if (!f) {
                fmt::print(stderr, "Unable to open source file");
            }
            log_info("Found source code");
            source.RegisterSourceFile(ParseSourceFile(f));
        }
    
        std::thread t86vm([](std::unique_ptr<ThreadMessenger> messenger,
                           tiny::t86::Program program, size_t reg_cnt,
                           double float_reg_cnt) {
            tiny::t86::OS os(reg_cnt, float_reg_cnt);
            os.SetDebuggerComms(std::move(messenger));
            os.Run(std::move(program));
        }, std::move(tm1), std::move(program), reg_count, float_reg_count);
        
        auto t86dbg = std::make_unique<T86Process>(std::move(tm2), reg_count,
                                                   float_reg_count);
        Native native(std::move(t86dbg));
        CliInstance cli(std::move(native), std::move(source));
        cli.Run();
        t86vm.join();
    }
#endif

}
