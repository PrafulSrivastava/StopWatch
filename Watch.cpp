#include <iostream>
#include <boost/statechart/simple_state.hpp>
#include <boost/statechart/state_machine.hpp>
#include <boost/statechart/transition.hpp>
#include <thread>
#include <windows.h>

#define START_STOP_KEY 83
#define RESET_KEY 82
#define EXIT_KEY 84

namespace clk = std::chrono;
typedef clk::steady_clock::time_point timept;
namespace sc = boost::statechart;
namespace mpl = boost::mpl;

double base_time = 0;
static HANDLE handle_in = NULL;
static HANDLE handle_out = NULL;

struct IElapsedTime {
	virtual double get_time_elapsed() const = 0;
};

struct Ev_start_stop : sc::event<Ev_start_stop> {
	Ev_start_stop() {
		//std::cout << "Ev_start_stop triggered!\n";
	}
};
struct Ev_reset : sc::event<Ev_reset> {
	Ev_reset() {
		//std::cout << "Ev_reset triggered!\n";
	}
};

struct Active;
struct StopWatch : sc::state_machine<StopWatch, Active> {
	double get_elapsed_time() const {
		try {
			auto res = state_cast<const IElapsedTime&>().get_time_elapsed();
			return res;
		}
		catch (...) {
			return 0;
		}
	}
};
struct Stopped;
struct Active : sc::simple_state<Active, StopWatch, Stopped> {
	Active() : elapsed_time { 0.0 } {
		//std::cout << "Active state!\n";
	}
	double get_time_elapsed() const {
		return elapsed_time;
	}
	double& get_time_elapsed_ref() {
		return elapsed_time;
	}
	typedef mpl::list< sc::transition<Ev_start_stop, Active>, 
	sc::transition<Ev_reset, Active>> reactions;
private:
	double elapsed_time;
};
struct Running : IElapsedTime, sc::simple_state<Running, Active> {
	Running() : start_time{ timept() } {
		//std::cout << "Running sub state!\n";
	}
	~Running() {
		context<Active>().get_time_elapsed_ref() = get_time_elapsed();
	}
	virtual double get_time_elapsed() const {
		timept now = std::chrono::steady_clock::now();
		double diff = clk::duration_cast<clk::seconds>(now - start_time).count();
		return diff;
	}
	typedef sc::transition<Ev_start_stop, Stopped> reactions;
private:
	timept start_time;
};
struct Stopped :IElapsedTime, sc::simple_state<Stopped, Active> {
	Stopped() {
		//std::cout << "Stopped sub state!\n";
	}
	virtual double get_time_elapsed() const {
		return context<Active>().get_time_elapsed();
	}
	typedef sc::transition<Ev_start_stop, Running> reactions;
};
void gotoxy(int x, int y) {
	if (!handle_out)
		handle_out = GetStdHandle(STD_OUTPUT_HANDLE);
	COORD c = { x, y };
	SetConsoleCursorPosition(handle_out, c);
}

void hide_cursor() { 	
	if (!handle_out)
		handle_out = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_CURSOR_INFO info;  
	GetConsoleCursorInfo(handle_out, &info);
	info.bVisible = FALSE;
	SetConsoleCursorInfo(handle_out, &info); 
}

void counter(StopWatch& watch, bool run_cntr) {
	while (run_cntr) {
		gotoxy(20, 4);
		double res = watch.get_elapsed_time();
		printf("%lf ",res - base_time);
		gotoxy(20, 4);
	}
}

void display_menu() {
	std::cout << "(S) . Start/Stop" << std::endl;
	std::cout << "(R) . Reset" << std::endl;
	std::cout << "(T) . Exit" << std::endl;
}

void hide_input() {
	if (!handle_in)
		handle_in = GetStdHandle(STD_INPUT_HANDLE);
	DWORD mode = 0;
	GetConsoleMode(handle_in, &mode);
	SetConsoleMode(handle_in, mode & (~ENABLE_ECHO_INPUT));
}

int main() {
	StopWatch watch;
	watch.initiate();
	bool run_cntr = true;
	bool start_flag = true;
	bool set_base = false;
	display_menu();
	hide_cursor();
	hide_input();
	std::thread t1(counter, std::ref(watch), std::ref(run_cntr));
	char temp;
	while(start_flag){
			temp = getchar();
			switch (temp) {
			case START_STOP_KEY:
				watch.process_event(Ev_start_stop());
				if (!set_base) {
					base_time = watch.get_elapsed_time();
					set_base = true;
				}
				break;
			case RESET_KEY:
				watch.process_event(Ev_reset());
				base_time = watch.get_elapsed_time();
				set_base = false;
				break;
			case EXIT_KEY:
				std::cout << "Exiting!" << std::endl;
				start_flag = false;
				run_cntr = false;
				break;
			}
		
	}
	t1.join();
	return 0;
}
