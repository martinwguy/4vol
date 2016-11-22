/**  4vol, a single-speaker mono audio power quadrupler
 *
 * With one loudspeaker and a stereo output you can get four times the
 * audio power (twice what you'd get with a stereo pair) by connecting the
 * speaker to the positive terminals of the left and right channels and using
 * this plugin in the signal path. It averages the two channels into one mono
 * channel and puts plus that on the left channel and minus that on the right
 * thereby doubling the voltage to the speaker, wuich quadruples the power.
 *
 * I hope your sound card can stand the higher current without burning out!
 *
 * Made by adapting simple_client.c from github.com/jackaudio/example-clients
 *
 * Compile:
 *	cc -O2 -o 4vol 4vol.c -ljack
 * Run:
 *	Launch qjackctl and start the JACK daemon
 *	./v4ol &
 *	Start the audio program you want to use, e.g.
 *	rezound --audio-method=jack
 * Open qjackctl's Connection bay, Disconnect All, click-drag the
 *	program's Output Ports to 4vol's Input Ports and click-drag
 *	4vol's Output Ports to the System Input (playback) Ports.
 * Now hit play on your audio application.
 *
 *	Martin Guy <martinwguy@gmail.com>, November 2016.
 */

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <jack/jack.h>

jack_port_t *front_left_in, *front_right_in;
jack_port_t *front_left_out, *front_right_out;
jack_client_t *client;

/**
 * The process callback for this JACK application is called in a
 * special realtime thread once for each audio cycle.
 *
 * This client does nothing more than copy data from its input
 * port to its output port. It will exit when stopped by 
 * the user (e.g. using Ctrl-C on a unix-ish operating system)
 */
int
process (jack_nframes_t nframes, void *arg)
{
	jack_default_audio_sample_t *left_in, *left_out;
	jack_default_audio_sample_t *right_in, *right_out;
	int i;
	
	left_in = jack_port_get_buffer (front_left_in, nframes);
	right_in = jack_port_get_buffer (front_right_in, nframes);
	left_out = jack_port_get_buffer (front_left_out, nframes);
	right_out = jack_port_get_buffer (front_right_out, nframes);
	for (i=nframes; i>0; i--) {
		float mono = (*left_in++ + *right_in++)/2;
		*left_out++ = mono; *right_out++ = -mono;
	}

	return 0;      
}

/**
 * JACK calls this shutdown_callback if the server ever shuts down or
 * decides to disconnect the client.
 */
void
jack_shutdown (void *arg)
{
	exit (1);
}

int
main (int argc, char *argv[])
{
	const char **ports;
	const char *client_name = "4vol";
	const char *server_name = NULL;
	jack_options_t options = JackNullOption;
	jack_status_t status;
	
	/* open a client connection to the JACK server */

	client = jack_client_open (client_name, options, &status, server_name);
	if (client == NULL) {
		fprintf (stderr, "jack_client_open() failed, "
			 "status = 0x%2.0x\n", status);
		if (status & JackServerFailed) {
			fprintf (stderr, "Unable to connect to JACK server\n");
		}
		exit (1);
	}
	if (status & JackServerStarted) {
		fprintf (stderr, "JACK server started\n");
	}
	if (status & JackNameNotUnique) {
		client_name = jack_get_client_name(client);
		fprintf (stderr, "unique name `%s' assigned\n", client_name);
	}

	/* tell the JACK server to call `process()' whenever
	   there is work to be done.
	*/

	jack_set_process_callback (client, process, 0);

	/* tell the JACK server to call `jack_shutdown()' if
	   it ever shuts down, either entirely, or if it
	   just decides to stop calling us.
	*/

	jack_on_shutdown (client, jack_shutdown, 0);

	/* create input and output ports */

	front_left_in = jack_port_register (client, "front-left",
					 JACK_DEFAULT_AUDIO_TYPE,
					 JackPortIsInput, 0);
	front_right_in = jack_port_register (client, "front-right",
					 JACK_DEFAULT_AUDIO_TYPE,
					 JackPortIsInput, 0);

	front_left_out = jack_port_register (client, "front-left-out",
					  JACK_DEFAULT_AUDIO_TYPE,
					  JackPortIsOutput, 0);
	front_right_out = jack_port_register (client, "front-right-out",
					  JACK_DEFAULT_AUDIO_TYPE,
					  JackPortIsOutput, 0);

	if (front_left_in == NULL || front_right_in == NULL ||
	    front_left_out == NULL || front_right_out == NULL) {
		fprintf(stderr, "no more JACK ports available\n");
	        if (front_left_in == NULL) printf("No front_left_in\n");
	        if (front_right_in == NULL) printf("No front_right_in\n");
	        if (front_left_out == NULL) printf("No front_left_out\n");
	        if (front_right_out == NULL) printf("No front_right_out\n");
		exit (1);
	}

	/* Tell the JACK server that we are ready to roll.  Our
	 * process() callback will start running now. */

	if (jack_activate (client)) {
		fprintf (stderr, "cannot activate client");
		exit (1);
	}

#if 0
	/* Connect the ports.  You can't do this before the client is
	 * activated, because we can't make connections to clients
	 * that aren't running.  Note the confusing (but necessary)
	 * orientation of the driver backend ports: playback ports are
	 * "input" to the backend, and capture ports are "output" from
	 * it.
	 */

	ports = jack_get_ports (client, NULL, NULL,
				JackPortIsPhysical|JackPortIsOutput);
	if (ports == NULL) {
		fprintf(stderr, "no physical capture ports\n");
		exit (1);
	}

	if (jack_connect (client, ports[0], jack_port_name (input_port))) {
		fprintf (stderr, "cannot connect input ports\n");
	}

	free (ports);
	
	ports = jack_get_ports (client, NULL, NULL,
				JackPortIsPhysical|JackPortIsInput);
	if (ports == NULL) {
		fprintf(stderr, "no physical playback ports\n");
		exit (1);
	}

	if (jack_connect (client, jack_port_name (output_port), ports[0])) {
		fprintf (stderr, "cannot connect output ports\n");
	}

	free (ports);
#endif

	/* keep running until stopped by the user */

	sleep (-1);

	/* this is never reached but if the program
	   had some other way to exit besides being killed,
	   they would be important to call.
	*/

	jack_client_close (client);
	exit (0);
}
