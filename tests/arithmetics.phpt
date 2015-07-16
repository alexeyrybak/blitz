--TEST--
testing arithmetics
--FILE--
<?php
include('common.inc');
ini_set('blitz.remove_spaces_around_context_tags', 1);

$body = <<<BODY
{{ IF a % 2 == 1 }}odd{{ ELSE }}even{{ END }}
{{ IF b % 2 == 0 }}even{{ ELSE }}odd{{ END }}

{{ IF 6 + 5 * 20 - 4 % 7 / 2 == 104 }}correct!{{ ELSE }}math failure{{ END }}
{{ IF 104 == 6 + 5 * 20 - 4 % 7 / 2 }}correct!{{ ELSE }}math failure{{ END }}
{{ IF 6 + 5 * 20 - 4 % 7 / 2 == 6 + 5 * 20 - 4 % 7 / 2 }}correct!{{ ELSE }}math failure{{ END }}
{{ IF 3 + 2 == 5 }}correct!{{ ELSE }}math failure{{ END }}
{{ IF (3 + 2) == 5 }}correct!{{ ELSE }}math failure{{ END }}
{{ IF (3 + (2)) == 5 }}correct!{{ ELSE }}math failure{{ END }}
{{ IF 3 + 2 * 3 == 9 }}correct!{{ ELSE }}math failure{{ END }}
{{ IF 3 + 2 + 3 * 4 == 17 }}correct!{{ ELSE }}math failure{{ END }}
{{ IF 3 * 2 + 3 + 4 == 13 }}correct!{{ ELSE }}math failure{{ END }}
{{ IF 3 + 2 * 3 + 4 * 5 == 29 }}correct!{{ ELSE }}math failure{{ END }}
{{ IF (3 + 2) * 3 == 15 }}correct!{{ ELSE }}math failure{{ END }}
{{ IF 3 * (2 + 3) == 15 }}correct!{{ ELSE }}math failure{{ END }}
{{ IF 3 * 2 + 3 * (5 + 2) == 27 }}correct!{{ ELSE }}math failure{{ END }}
{{ IF 3 * 2 + 3 * (5 + 2 * 3) == 39 }}correct!{{ ELSE }}math failure{{ END }}
{{ IF ((3 + 2) * 5) == 25 }}correct!{{ ELSE }}math failure{{ END }}
{{ IF ((3 - 1) / 2) * 3 + 5 == 8 }}correct!{{ ELSE }}math failure{{ END }}
{{ IF ((3 - 1) / 2) * (3 + 5) == 8 }}correct!{{ ELSE }}math failure{{ END }}
{{ IF ((((1 * (2 + 3)) - 3) + 4) * 5) == 30 }}correct!{{ ELSE }}math failure{{ END }}

{{ BEGIN el }}{{ v }} {{ IF _num %3 == 0}}
{{ END }}
{{ END el }}


{{ IF six + five * twenty - four % seven / twoo == 104 }}correct!{{ ELSE }}math failure{{ END }}

BODY;

$T = new Blitz();
$T->load($body);

$T->display(
	array(
		'a' => '1',
		'b' => '2',
		'el' => array(
			array('v' => 0),
			array('v' => 1),
			array('v' => 2),
			array('v' => 3),
			array('v' => 4),
			array('v' => 5),
			array('v' => 6)
		),
		'twoo' => 2,
		'four' => 4,
		'five' => '5',
		'six' => 6.0,
		'seven' => 0x7,
		'twenty' => '2' . '0'
	)
);
?>
--EXPECT--
odd
even

correct!
correct!
correct!
correct!
correct!
correct!
correct!
correct!
correct!
correct!
correct!
correct!
correct!
correct!
correct!
correct!
correct!
correct!

0 1 2 
3 4 5 
6 

correct!
