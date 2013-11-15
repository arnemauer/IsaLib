#############################################################################
# ICR/notes generator
# This program creates C header file containing 
# notes and ICRx values for use with 16 bit timer in 
# Phase/Frequency correct PWM mode (AVR ATmega microcontrollers)
# (C) 2008 Aquaticus.info
# 
# USAGE: notegen.pl clock_MHz prescaler >output.h
#
# $Release$
#############################################################################

if( scalar(@ARGV) != 2)
{
	use Pod::Usage;
	pod2usage( { 
		     -exitval => 2  , 
		     -verbose => 2 } );

	exit;
}

my $sysclock = $ARGV[0] * 1000000;
my $prescaler = $ARGV[1];

my $DEFH = "__PWM_NOTES_H__"; #C preprocesor def
my $MAXICR = 0xFFFF; #max value for ICR
my $octaves = 6; #number of octaves to generate
my $base_frequency = 110; #frequency for A note in $base_octave
my $base_octave = 2; #octave number 

# "!" -> will be replaced with octave number
# use upper case letters (lower case version will be generated)
%NOTES = (
	"C!", -9,
	"C!x",-8,
	"Cis!",-8,
	"D!b",-8,
	"D!", -7,
	"D!x",-6,
	"Dis!",-6,
	"E!b",-6,
	"E!", -5,
	"F!", -4,
	"F!x",-3,
	"Fis!",-3,
	"G!b",-3,
	"G!", -2,
	"G!x",-1,
	"Gis!",-1,
	"A!b",-1,
	"A!", 0,
	"A!x",1,
	"Ais!",1,
	"B!b", 1,
	"B!", 2,
	
	
	#extra note names, e.g. H is used instead of B in some countries
	"H!",2,
	"H!b", 1,
	"bH!", 1,
	"xA!",1,
	"xC!",-8,
	"xF!",-3,
	"xG!",-2,
	"B!b", 1,
	"A!b",-1,
	"G!b",-3,
	"E!b",-6,
	"D!b",-8,
	
	#if you prefer other note names e.g. Do, Re, Mi, Fa, Sol... add them here
);
	

printf STDERR "System clock: %.0f Hz, Prescaler: %d\n", $sysclock, $prescaler;

print <<"HEADER";
/*
ICR values for AVR micro controllers to play sound using PWM.
System clock:   $sysclock [Hz]
Prescaler:      $prescaler
PWM mode:       phase/frequency correct 16 bit

This file was automatically generated using script from 
http://aquaticus.info/pwm-music
*/

#ifndef $DEFH
#define $DEFH

/* Pause */

#define P 1
#define PAUSE P
#define p P

/* end of notes table */
#define MUSIC_END 0

/* There are 12 notes in octave + variations (lower case, other names)  */
HEADER

for( my $octave=0; $octave<$octaves; $octave++ )
{
	$octave_nr = $base_octave+$octave;
	print "\n/* Octave #$octave_nr */\n\n";

	foreach $name (sort keys %NOTES)
	{
		$index = $NOTES{$name};
	
		# Frequency = base * 2^(tone_nr/12)
		$note_frequency = $base_frequency * (2.0**(($index+$octave*12.0)/12.0));
		
		#float ICRx value
		$icrf = $sysclock/($note_frequency * 2.0 * $prescaler);

		#round float ICR value to the nearest integer (Perl have no round() function)
		$icr = int($icrf + .5);
		
		#this is real frequency generated for note; based on integer ICR value
		$real_freq = $sysclock/(2*$prescaler*$icr);
		
		#error in %
		$error = abs($real_freq-$note_frequency )/$real_freq * 100;
		
		#replace '!' with octave number
		$name =~ s/!/$octave_nr/;
		
		if( $icr > 0 && $icr < $MAXICR )
		{
			#print 2 times, for the second time low case is used
			for($lc=0;$lc<2;$lc++)
			{
				printf "#define %-4s %-4d \t/* PWM: %.2f Hz, note freq: %.2f Hz, error %.2f%% */\n",	
																					$name,
																					$icr, 
																					$real_freq, 
																					$note_frequency,
																					$error;
																					
				$name = lc $name;
			}
			#lower case (format string must be tha same as above)

		}
		else
		{
			print "/* Note $name can not be played with current system clock and prescaler */\n";
		}
	}
}

print <<"TRAILER";

#endif /* $DEFH */
TRAILER

__END__

=head1 NAME 

notegen.pl - generate C header file with music notes for AVR microcontroller

=head1 SYNOPSIS

mkdld.pl sys_clock_mhz prescaler

=head1 DESCRIPTION

Script create C header file that contains numbers to generate appropriate music notes 
using PWM channel of Atmel AVR microcontrollers.

=over 

=item B<sys_clock_mhz>

System clock in MHz; 1MHz = 1,000,000 Hz, e.g. 1 for 1MHz, 8.192 for 8.192MHz.

=item B<prescaler>

Prescaler dvider. For timer1 of ATmega16 and ATmega32 microcontrollers it can be: 1, 8, 64, 256, 1024.
The value of prescaler must be valid for target microcontroller and timer.

=back

Script first compute frequency of the note, then value that needs to be written to ICRx
register to generate PWM wave.

12 notes for 5 octaves are generated. For example for octave 4 notes are:
B<A4, A4x, B4, C4, C4x, D4, D4x, E4, F4, F4x, G4, G4x>.

In addition other name varians are generated:
a4, a4x, b4, c4, c4x, d4, d4x, e4, f4, f4x, g4, g4x, H4, h4, xA4, xa4, xC4,
xc4, xD4, xd4, xF4, xf4, xG4, xg4.


=head1 EXAMPLES

C<notegen.pl 1 1 E<gt>notes.h>

Generate header file F<notes.h> for 1MHz and prescaler divider 8.

C<notegen.pl 8 1 E<gt>notes.h>

Generate header file F<notes.h> for 8MHz and prescaler divider 1.

=head1 EXAMPLE OUTPUT

 #define D3   1703 /* PWM: 293.60 Hz, note freq: 293.66 Hz, error 0.02% */
 #define d3   1703 /* PWM: 293.60 Hz, note freq: 293.66 Hz, error 0.02% */
 #define A3   2273 /* PWM: 219.97 Hz, note freq: 220.00 Hz, error 0.01% */
 #define a3   2273 /* PWM: 219.97 Hz, note freq: 220.00 Hz, error 0.01% */
 #define xG3  1204 /* PWM: 415.28 Hz, note freq: 415.30 Hz, error 0.01% */

=head1 HOW TO USE

First configure AVR timer for Phase/Frequency correct PWM mode.
To play notes, write appropriate note to ICRx register, e.g B<ICR1A=D3;> to generate D3 note.

=head1 AUTHOR

(c) Aquaticus.info 2008 L<http://aquaticus.info>

=head1 LICENCE

You can use this script and files generated for commercial and non-commercial purposes.


