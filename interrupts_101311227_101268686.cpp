/**
 *
 * @file interrupts.cpp
 * @author Sasisekhar Govind
 *
 */

#include <interrupts_101311227_101268686.hpp>

std::tuple<std::string, std::string, int> simulate_trace(std::vector<std::string> trace_file, int time, std::vector<std::string> vectors, std::vector<int> delays, std::vector<external_file> external_files, PCB current, std::vector<PCB> wait_queue)
{

    std::string trace;              //!< string to store single line of trace file
    std::string execution = "";     //!< string to accumulate the execution output
    std::string system_status = ""; //!< string to accumulate the system status output
    int current_time = time;

    // parse each line of the input trace file. 'for' loop to keep track of indices.
    for (size_t i = 0; i < trace_file.size(); i++)
    {
        auto trace = trace_file[i];

        auto [activity, duration_intr, program_name] = parse_trace(trace);

        if (activity == "CPU")
        { // As per Assignment 1
            execution += std::to_string(current_time) + ", " + std::to_string(duration_intr) + ", CPU Burst\n";
            current_time += duration_intr;
        }
        else if (activity == "SYSCALL")
        { // As per Assignment 1
            auto [intr, time] = intr_boilerplate(current_time, duration_intr, 10, vectors);
            execution += intr;
            current_time = time;

            execution += std::to_string(current_time) + ", " + std::to_string(delays[duration_intr]) + ", SYSCALL ISR\n";
            current_time += delays[duration_intr];

            execution += std::to_string(current_time) + ", 1, IRET\n";
            current_time += 1;
        }
        else if (activity == "END_IO")
        {
            auto [intr, time] = intr_boilerplate(current_time, duration_intr, 10, vectors);
            current_time = time;
            execution += intr;

            execution += std::to_string(current_time) + ", " + std::to_string(delays[duration_intr]) + ", ENDIO ISR\n";
            current_time += delays[duration_intr];

            execution += std::to_string(current_time) + ", 1, IRET\n";
            current_time += 1;
        }
        else if (activity == "FORK")
        {
            auto [intr, time] = intr_boilerplate(current_time, 2, 10, vectors);
            execution += intr;
            current_time = time;

            ///////////////////////////////////////////////////////////////////////////////////////////
            // Add your FORK output here

            PCB child = current;
            child.PID = current.PID + 1;
            child.PPID = current.PID;

            child.partition_number = -1;
            child.program_name = current.program_name;
            child.size = current.size;
            execution += std::to_string(current_time) + ", " + std::to_string(duration_intr) + ", cloning the PCB\n";
            current_time += duration_intr;

            bool child_alloc = allocate_memory(&child);
            if (!child_alloc)
            {
                execution += std::to_string(current_time) + ", 0, no free parition for child, continuing for parent\n";
                execution += std::to_string(current_time) + ", 0, scheduler called\n";
                execution += std::to_string(current_time) + ", 1, IRET\n";
                current_time += 1;
                std::vector<PCB> snapshot_q = wait_queue;
                system_status += "time: " + std::to_string(current_time) + "; current trace: FORK, " + std::to_string(duration_intr) + "\n" + print_PCB(current, snapshot_q) + "\n";
            }
            else
            {
                wait_queue.push_back(current);
                execution += std::to_string(current_time) + ", 0, scheduler called\n" + std::to_string(current_time) + ", 1, IRET\n";
                current_time += 1;
                std::vector<PCB> snapshot_q = wait_queue;
                system_status += "time: " + std::to_string(current_time) + "; current trace: FORK, " + std::to_string(duration_intr) + "\n" + print_PCB(child, snapshot_q) + "\n";
            }

            ///////////////////////////////////////////////////////////////////////////////////////////

            // The following loop helps you do 2 things:
            //  * Collect the trace of the child (and only the child, skip parent)
            //  * Get the index of where the parent is supposed to start executing from
            std::vector<std::string> child_trace;
            bool skip = true;
            bool exec_flag = false;
            int parent_index = 0;

            for (size_t j = i; j < trace_file.size(); j++)
            {
                auto [_activity, _duration, _pn] = parse_trace(trace_file[j]);
                if (skip && _activity == "IF_CHILD")
                {
                    skip = false;
                    continue;
                }
                else if (_activity == "IF_PARENT")
                {
                    skip = true;
                    parent_index = j;
                    if (exec_flag)
                    {
                        break;
                    }
                }
                else if (skip && _activity == "ENDIF")
                {
                    skip = false;
                    continue;
                }
                else if (!skip && _activity == "EXEC")
                {
                    skip = true;
                    child_trace.push_back(trace_file[j]);
                    exec_flag = true;
                }

                if (!skip)
                {
                    child_trace.push_back(trace_file[j]);
                }
            }
            i = parent_index;

            ///////////////////////////////////////////////////////////////////////////////////////////
            // With the child's trace, run the child (HINT: think recursion) (post-fork)

            if (child_alloc && !child_trace.empty())
            {
                // Run the child process recursively
                auto result = simulate_trace(child_trace, current_time, vectors, delays, external_files, child, wait_queue);
                auto &[exec_out, status_out, finish_time] = result;

                // Append child output to parent
                execution += exec_out;
                system_status += status_out;
                current_time = finish_time;

                if (child.partition_number >= 0)
                {
                    free_memory(&child);
                }

                auto remove_by_pid = [&](const PCB &proc)
                {
                    return proc.PID == child.PID || proc.PID == current.PID;
                };
                wait_queue.erase(std::remove_if(wait_queue.begin(), wait_queue.end(), remove_by_pid), wait_queue.end());
            }

            ///////////////////////////////////////////////////////////////////////////////////////////
        }
        else if (activity == "EXEC")
        {
            auto [intr, time] = intr_boilerplate(current_time, 3, 10, vectors);
            current_time = time;
            execution += intr;

            ///////////////////////////////////////////////////////////////////////////////////////////
            // Add your EXEC output here

            unsigned int program_size = get_size(program_name, external_files);
            if (program_size == (unsigned int)-1)
                program_size = 1;
            int load_time = program_size * 15;

            execution += std::to_string(current_time) + ", " + std::to_string(duration_intr) + ", Program is " + std::to_string(program_size) + "Mb large\n";
            current_time += duration_intr;

            execution += std::to_string(current_time) + ", " + std::to_string(load_time) + ", loading program into memory\n";
            current_time += load_time;

            if (current.partition_number != -1)
            {
                free_memory(&current);
            }

            current.program_name = program_name;
            current.size = program_size;
            allocate_memory(&current);

            // Partition Occupied
            execution += std::to_string(current_time) + ", 3, marking parition as occupied\n";
            current_time += 3;
            execution += std::to_string(current_time) + ", 6, updating PCB\n";
            current_time += 6;

            // Scheduler, IRET
            execution += std::to_string(current_time) + ", 0, scheduler called\n";
            execution += std::to_string(current_time) + ", 1, IRET\n";
            current_time += 1;

            // Status
            std::vector<PCB> queue_image = wait_queue;
            queue_image.erase(
                std::remove_if(queue_image.begin(), queue_image.end(), [&](const PCB &p)
                               { return p.PID == current.PID; }),
                queue_image.end());

            system_status += "time: " + std::to_string(current_time) + "; current trace: EXEC " + program_name + ", " + std::to_string(duration_intr) + "\n" + print_PCB(current, queue_image) + "\n";

            ///////////////////////////////////////////////////////////////////////////////////////////

            std::ifstream exec_trace_file(program_name + ".txt");

            std::vector<std::string> exec_traces;
            std::string exec_trace;
            while (std::getline(exec_trace_file, exec_trace))
            {
                exec_traces.push_back(exec_trace);
            }

            ///////////////////////////////////////////////////////////////////////////////////////////
            // With the exec's trace (i.e. trace of external program), run the exec (HINT: think recursion)

            if (!exec_traces.empty())
            {
                auto [prog_exec, prog_status, prog_time] = simulate_trace(exec_traces, current_time, vectors, delays, external_files, current, wait_queue);
                execution += prog_exec;
                system_status += prog_status;
                current_time = prog_time;
            }

            ///////////////////////////////////////////////////////////////////////////////////////////

            break; // Why is this important? (answer in report)
        }
    }

    return {execution, system_status, current_time};
}

int main(int argc, char **argv)
{

    // vectors is a C++ std::vector of strings that contain the address of the ISR
    // delays  is a C++ std::vector of ints that contain the delays of each device
    // the index of these elemens is the device number, starting from 0
    // external_files is a C++ std::vector of the struct 'external_file'. Check the struct in
    // interrupt.hpp to know more.
    auto [vectors, delays, external_files] = parse_args(argc, argv);
    std::ifstream input_file(argv[1]);

    // Just a sanity check to know what files you have
    print_external_files(external_files);

    // Make initial PCB (notice how partition is not assigned yet)
    PCB current(0, -1, "init", 1, -1);
    // Update memory (partition is assigned here, you must implement this function)
    if (!allocate_memory(&current))
    {
        std::cerr << "ERROR! Memory allocation failed!" << std::endl;
    }

    std::vector<PCB> wait_queue;

    /******************ADD YOUR VARIABLES HERE*************************/

    /******************************************************************/

    // Converting the trace file into a vector of strings.
    std::vector<std::string> trace_file;
    std::string trace;
    while (std::getline(input_file, trace))
    {
        trace_file.push_back(trace);
    }

    auto [execution, system_status, _] = simulate_trace(trace_file,
                                                        0,
                                                        vectors,
                                                        delays,
                                                        external_files,
                                                        current,
                                                        wait_queue);

    input_file.close();

    write_output(execution, "output_files/execution.txt");
    write_output(system_status, "output_files/system_status.txt");

    return 0;
}
