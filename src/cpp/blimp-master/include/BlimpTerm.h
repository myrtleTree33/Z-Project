#ifndef BLIMPTERM_H
#define BLIMPTERM_H

#include <boost/asio.hpp>
#include <boost/ref.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

class BlimpTerm
{
	public:
		BlimpTerm();
		virtual ~BlimpTerm();
		void doWrite(unsigned char arg);

	protected:
	private:
		boost::asio::serial_port_base::baud_rate 				*BAUD;
		boost::asio::serial_port_base::character_size		*SIZE;
		boost::asio::serial_port_base::flow_control 			*FLOW;
		boost::asio::serial_port_base::parity 					*PARITY;
		boost::asio::serial_port_base::stop_bits				*STOP;

		boost::asio::io_service 									*io;
		boost::asio::serial_port 									*port;
		boost::asio::deadline_timer 								*timeout;

		const char *PORT;
		unsigned char inBuffer[256];
		bool dataAvailable;

#endif // BLIMPTERM_H
