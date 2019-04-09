##### NAME

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;**rawserv** - raw HTTP file server

##### SYNOPSIS

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;**rawserv** port root [max_path_length=256]

##### DESCRIPTION

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;**rawserv** serves raw HTTP files requested with the GET method relatively to the root and shorter than max_path_length characters on a port.

##### EXIT STATUS
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;0  the server was closed with a signal.

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;1  the arguments could not be processed.

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;2  the signal handler could not be set.

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;3  the socket could not be created, binded or listened to.

##### CONFORMING TO
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;RFC2616