#include <iostream>
#include <cstdio>

#include <ncurses.h>
#include <boost/asio.hpp>
#include <boost/ref.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
//#include "curl/curl.h"

using namespace std;

const char *PORT = "/dev/ttyACM0";

//using namespace LibSerial;

/* CURL stuff */
/*
int writer(char *data, size_t size, size_t nmemb, std::string *writerData) {
  if (writerData == NULL)
    return 0;

  writerData->append(data, size*nmemb);
  return size * nmemb;
}

*/

void read_callback(bool& dataAvailable, boost::asio::deadline_timer &timeout, const boost::system::error_code& error, std::size_t bytes_transferred) {
	if (error || !bytes_transferred) {
		dataAvailable = false;
		return;
	}
	timeout.cancel();
	dataAvailable = true;
	printw("data is available");
}

void wait_callback(boost::asio::serial_port& ser_port, const boost::system::error_code& error) {
	if (error) {
		return;
	}
	ser_port.cancel();
}


int main() {

/* // CURL stuff

CURL			*curl;
CURLcode		res;
std::string	buffer;

curl = curl_easy_init();

if (curl) {
	curl_easy_setopt(curl, CURLOPT_URL, "http://search.twitter.com/search.atom?q=%23blimpme");
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writer);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
	res = curl_easy_perform(curl);
	cout << buffer << endl;
	curl_easy_cleanup(curl);
}
*/

	char ch;
	initscr();
	nodelay(stdscr, TRUE);
	raw();
	noecho();

	boost::asio::serial_port_base::baud_rate 				BAUD(9600);
	boost::asio::serial_port_base::character_size		SIZE(8);
	boost::asio::serial_port_base::flow_control 			FLOW(boost::asio::serial_port_base::flow_control::none);
	boost::asio::serial_port_base::parity 					PARITY(boost::asio::serial_port_base::parity::none);
	boost::asio::serial_port_base::stop_bits				STOP(boost::asio::serial_port_base::stop_bits::one);

	boost::asio::io_service io;
	boost::asio::serial_port port(io, PORT);
	boost::asio::deadline_timer* timeout = new boost::asio::deadline_timer(io);

	unsigned char inBuffer[512];
	bool dataAvailable = false;

	port.set_option( BAUD );
	port.set_option( SIZE );
	port.set_option( FLOW );
	port.set_option( PARITY );
	port.set_option( STOP );

		port.async_read_some(boost::asio::buffer(inBuffer), boost::bind(&read_callback, boost::ref(dataAvailable), boost::ref(*timeout), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
		timeout->expires_from_now(boost::posix_time::milliseconds(50));
		timeout->async_wait(boost::bind(&wait_callback, boost::ref(port), boost::asio::placeholders::error));

	io.run();

	while (1) {



		if (dataAvailable) {
			printw("here it is");
			dataAvailable = false;
		} else {
			//printw("%c", 'a');
		}
		ch = getch();
		if (ch != ERR) {
			unsigned char command[1] = {'0'};
			command[0] = static_cast<unsigned char>(ch);
			write(port, boost::asio::buffer(command, 1) );
			printw("%s", "i am here");
			printw("%c",ch);
		}

		refresh();
	}


	return 0;
}

