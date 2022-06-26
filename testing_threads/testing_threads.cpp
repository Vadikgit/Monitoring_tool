// testing_threads.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include <iostream>
#include <chrono>
#include <thread>
#include <string>
#include <sstream>
#include <mutex>
#include <vector>
#include <filesystem>


namespace commands {
	const std::string c_quit = "\\q";
	const std::string c_help = "\\?";
	const std::string c_timer = "\\t";
	const std::string c_node = "\\n";
	const std::string c_active_list = "\\a";
	const std::string c_stop_thread = "\\s";
}

std::mutex print_mut;
std::vector<std::pair<std::thread*, std::string>> active_threads;
std::vector<bool> is_thread_active;

const std::string invitation_string = "\ntesting_threads@>";

void print(const std::string str) {
	print_mut.lock();
	std::cout << str << std::flush;
	print_mut.unlock();
}

void printActiveThreads() {
	std::stringstream print_str;
	print_str << "Active threads:\n";

	for (auto& i : active_threads)
	{
		print_str << "\r          ----------<" << i.second << "> [" << i.first->get_id() << "] ----------\n";
	}
	print(print_str.str());
}

void timerProcedure(const std::string& parameter_str) {
	std::stringstream params;
	int64_t interval_ms, control_period_ms;

	params << parameter_str;
	params >> interval_ms >> control_period_ms;

	int counter = 0;
	for (auto& i : active_threads)
	{
		if (i.first->get_id() == std::this_thread::get_id())
			break;

		counter++;
	}

	for (int i = 0; i < interval_ms / control_period_ms; i++)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(control_period_ms));
		std::stringstream printing_str;

		if (!is_thread_active[counter])
		{
			break;
		}

		printing_str << "\r\a          ----------<timer> [" << std::this_thread::get_id() << "] : Measured "
			<< (control_period_ms * (i + 1)) << " ms of " << interval_ms << "----------\n\n";
		print(printing_str.str());
		print(invitation_string);
	}

	for (auto& i : active_threads)
	{
		if (i.first->get_id() == std::this_thread::get_id())
		{
			i.first->detach();
			is_thread_active[counter] = false;
		}
	}
}

void nodeMonitoringProcedure(const std::string& parameter_str) {
	int counter = 0;
	for (auto& i : active_threads)
	{
		if (i.first->get_id() == std::this_thread::get_id())
			break;

		counter++;
	}
	std::string pth_str = parameter_str;
	if (parameter_str[0] == '\"')
	{
		pth_str.erase(0, 1);
		pth_str.erase(pth_str.length() - 1, 1);
	}

	std::filesystem::path monitoring_node(pth_str);

	std::string original_content = "";
	for (const auto& entry : std::filesystem::directory_iterator(monitoring_node)) {
		original_content += entry.path().string();
		original_content += '\n';
	}

	while (is_thread_active[counter])
	{
		std::string current_content = "";
		for (const auto& entry : std::filesystem::directory_iterator(monitoring_node)) {
			current_content += entry.path().string();
			current_content += '\n';
		}

		if (current_content != original_content)
		{
			std::stringstream printing_str;

			printing_str << "\r\a          ----------<node_monitor> [" << std::this_thread::get_id() << "] :  content of node \""
			<<	pth_str  << "\" was changed.\n\nBefore:\n\n" << original_content <<  "\n\nAfter:\n\n" << current_content;
			print(printing_str.str());
			print(invitation_string);
		}
		
	
	
		original_content = current_content;
	}

	

}

void stopThreadProcedure(const std::string& parameter_str) {
	int counter = 0;
	for (auto& i : active_threads)
	{
		std::stringstream cur_thrd_num;
		cur_thrd_num << i.first->get_id();
		if (cur_thrd_num.str() == parameter_str)
		{
			i.first->detach();
			is_thread_active[counter] = false;
			std::string coment_str = "Thread [" + parameter_str + "] is detached\n";
			print(coment_str);
		}
		counter++;
	}
	
	printActiveThreads();
}

void printHelp() {
	std::string help_str = "Using:\n";

	help_str += "\t";
	help_str += commands::c_quit;
	help_str += " - quit;\n";

	help_str += "\t";
	help_str += commands::c_help;
	help_str += " - help;\n";

	help_str += "\t";
	help_str += commands::c_timer;
	help_str += " [interval in milliseconds] [control period in milliseconds] - timer;\n";

	help_str += "\t";
	help_str += commands::c_node;
	help_str += " [path to node] - monitoring the list of files in node;\n";

	help_str += "\t";
	help_str += commands::c_active_list;
	help_str += " - the list of active threads;\n";

	help_str += "\t";
	help_str += commands::c_stop_thread;
	help_str += " [thread number] - stop target thread;\n\n";

	print(help_str);
}


void process_command(const std::string &command, const std::string& parameters) {
	if (command == commands::c_quit)
	{
		for (auto& i : active_threads)
		{
			i.first->detach();
		}
	}
	else if (command == commands::c_help)
	{
		printHelp();
	}
	else if (command == commands::c_stop_thread)
	{
		stopThreadProcedure(parameters);
	}
	else if (command == commands::c_active_list)
	{
		printActiveThreads();
	}
	else if (command == commands::c_node)
	{
		is_thread_active.push_back(true);
		active_threads.push_back(std::make_pair(new std::thread(nodeMonitoringProcedure, std::ref(parameters)), "node_monitor"));
	}
	else if(command == commands::c_timer)
	{
		is_thread_active.push_back(true);
		active_threads.push_back(std::make_pair(new std::thread(timerProcedure, std::ref(parameters)), "timer"));
	}
	else
	{
		print("Undefined comand. Enter \"\\?\" to get help");
	}
	//print(invitation_string);
}

int main()
{
	auto beg = std::chrono::steady_clock::now();
	
	

	std::string input_str;
	std::stringstream inp_str_stream;
	std::string command_str;
	std::string parameter_str;

	print(invitation_string);
	std::getline(std::cin, input_str);
	inp_str_stream << input_str;
	inp_str_stream >> command_str;
	std::getline(inp_str_stream, parameter_str);
	parameter_str.erase(0, 1);
	
	while (command_str != commands::c_quit)
	{		
		process_command(command_str, parameter_str);

		input_str.clear();
		print(invitation_string);
		std::getline(std::cin, input_str);
		inp_str_stream.clear();
		inp_str_stream << input_str;
		command_str.clear();
		inp_str_stream >> command_str;
		parameter_str.clear();
		std::getline(inp_str_stream, parameter_str);
		parameter_str.erase(0, 1);
	}
	
	auto end = std::chrono::steady_clock::now();
	auto interval = std::chrono::duration_cast<std::chrono::microseconds>(end - beg);
	std::cout << "\nProgramm finished after " << interval.count() << " microseconds since running\n";
	return 0;
}
