#include "include/BlimpTerm.h"

BlimpTerm::BlimpTerm() {
	//ctor

	*PORT =  = "/dev/ttyACM0";
	dataAvailable = false;

	boost::asio::serial_port_base::baud_rate 				BAUD(9600);
	boost::asio::serial_port_base::character_size		SIZE(8);
	boost::asio::serial_port_base::flow_control 			FLOW(boost::asio::serial_port_base::flow_control::none);
	boost::asio::serial_port_base::parity 					PARITY(boost::asio::serial_port_base::parity::none);
	boost::asio::serial_port_base::stop_bits				STOP(boost::asio::serial_port_base::stop_bits::one);

	*io 		= new boost::asio::io_service;
	*port		= new boost::asio::serial_port(*io, PORT);
	*timeout	= new boost::asio::deadline_timer(io);

	*port->set_option( BAUD );
	*port->set_option( SIZE );
	*port->set_option( FLOW );
	*port->set_option( PARITY );
	*port->set_option( STOP );


}

BlimpTerm::~BlimpTerm() {
	//dtor
}

void BlimpTerm::doWrite(unsigned char arg) {
	write(*port, boost::asio::buffer(arg,1));
}
