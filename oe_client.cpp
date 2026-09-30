#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <cstdlib>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <limits>
#include <memory>
#include <thread>

#include <fstream>
#include <iostream>
#include <print>
#include <chrono>

struct FrameHeader {
	std::uint16_t body_len{std::byteswap(static_cast<std::uint16_t>(41))};
	std::uint8_t msg_type{2};
	std::uint8_t version{1};
} frame_header;

class OrderParser {
public:
	OrderParser(int sock_fd) : file{"csv.csv"}, sock_fd{sock_fd} {
		if (!file) {
			std::println(std::cerr, "Invalid file read");
			exit(EXIT_FAILURE);
		}
		file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	}

	void parse_line() {
		//auto now {std::chrono::steady_clock::now()};
		//std::println(std::cerr, "previous parse_line() took {}", now - prev_time);
		//prev_time = now;
		read_ts();
		if (!is_trade()) {
			read_quote(1);
		}
		read_trade(1);
	}

	void parse_lines(int num_lines) {
		for (int i{0}; i < num_lines; ++i) {
			parse_line();
		}
	}

private:
	void read_ts() {
		std::string ts{};
		std::getline(file, ts, ',');

		std::uint64_t total{ 0 };
		total += static_cast<std::uint64_t>(std::stoll(ts.substr(0, 2))) * 3'600 * 1'000'000'000;
		total += static_cast<std::uint64_t>(std::stoll(ts.substr(3, 5))) * 60 * 1'000'000'000;
		total += static_cast<std::uint64_t>(std::stoll(ts.substr(6, 8))) * 1'000'000'000;
		total += static_cast<std::uint64_t>(std::stoll(ts.substr(9, 12))) * 1'000'000;
		ts_ns = total;

		if (is_first_read) {
			prev_ts_ns = total;
			is_first_read = false;
		}
	}

	bool is_trade() {
		// read quote or trade
		char q_or_t{};
		file >> q_or_t;
		file.ignore();

		// skip if quote
		if (q_or_t != 'T') {
			return false;
		}
		return true;
	}

	// save prev ask price and bid price
	void read_quote(int symbol_num) {
		// read symbol
		file.getline(symbol, 13, ',');

		// skip if not chosen symbol
		if (symbol[5] - static_cast<int>('0') != symbol_num) {
			std::println(std::cerr, "Skipped different symbol");
			file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			return;
		}
		
		// read bid
		double bid_px_d{};
		file >> bid_px_d;
		prev_bid_px = static_cast<std::int64_t>(bid_px_d * 10'000);
		file.ignore();
		file.ignore(std::numeric_limits<std::streamsize>::max(), ',');

		// read ask
		double ask_px_d{};
		file >> ask_px_d;
		prev_ask_px = static_cast<std::int64_t>(ask_px_d * 10'000);
		file.ignore();
		file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	}

	void read_trade(int symbol_num) {
		// read symbol
		file.getline(symbol, 13, ',');

		// skip if not chosen symbol
		if (symbol[5] - static_cast<int>('0') != symbol_num) {
			std::println(std::cerr, "Skipped different symbol");
			file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			return;
		}
        file.ignore(4);

		// read px and qty
		double px_d{};
		file >> px_d;
		px = static_cast<std::int64_t>(px_d * 10'000);
		if (px >= prev_ask_px) aggressor = 'B';
		else if (px <= prev_bid_px) aggressor = 'S';
		else aggressor = '?';
		file.ignore();

		file >> qty;
		file.ignore();

		++id;

		// std::println("Sleep for {}", std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::nanoseconds{ts_ns - prev_ts_ns}));
		// std::this_thread::sleep_for(std::chrono::nanoseconds{ts_ns - prev_ts_ns});
		// prev_ts_ns = ts_ns;

		std::println("Success! {} {} {} {}", symbol, ts_ns, px, qty);
		serialize();
		send_quote();
	}

	void serialize() {
		ts_ns = std::byteswap(ts_ns);
        qty = std::byteswap(qty);
		px = std::byteswap(px);
		id = std::byteswap(id);

		std::memcpy(frame, &frame_header, 4);
		std::memcpy(frame + 4, symbol, 12);
		std::memcpy(frame + 16, &ts_ns, 8);
		std::memcpy(frame + 24, &qty, 4);
		std::memcpy(frame + 28, &px, 8);
		std::memcpy(frame + 36, &aggressor, 1);
		std::memcpy(frame + 37, &id, 8);

		id = std::byteswap(id);
	}

	void send_quote() {
		if (ssize_t bytes_sent = send(sock_fd, (void*)frame, 45, 0); bytes_sent == -1) {
			std::println(std::cerr, "send failed");
			exit(EXIT_FAILURE);
		} else {
			std::println("sent {} bytes", bytes_sent);
		}
	}

	std::ifstream file{};
	int sock_fd{};
	char frame[45]{};
	bool is_first_read{true};
	std::uint64_t prev_ts_ns{};
	//std::chrono::time_point<std::chrono::steady_clock> prev_time {std::chrono::steady_clock::now()};

	std::int64_t prev_bid_px{std::numeric_limits<std::int64_t>::lowest()};
	std::int64_t prev_ask_px{std::numeric_limits<std::int64_t>::max()};

	char symbol[12]{};
	std::uint64_t ts_ns{};
	std::uint32_t qty{};
	std::int64_t  px{};
	char aggressor{};
    std::int64_t id {0};
};
