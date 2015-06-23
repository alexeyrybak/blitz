--TEST--
testing empty strings
--FILE--
<?php
include('common.inc');
ini_set('blitz.remove_spaces_around_context_tags', 1);

$body = <<<BODY
a: {{var_export(a,1)}}
b: {{var_export(b,1)}}
expression: (a == 'text') || !b
result: {{IF (a == 'text') || !b}}OK{{ELSE}}WRONG{{END}}

expression: (b == 'text') || !b
result: {{IF (b == 'text') || !b}}OK{{ELSE}}WRONG{{END}}

c: {{var_export(c,1)}}
expression: (c == 'text') || !c
result: {{IF (c == 'text') || !c}}WRONG{{ELSE}}OK{{END}}

expression: (c == 'text')
result: {{IF (c == 'text')}}WRONG{{ELSE}}OK{{END}}

expression: !c
result: {{IF !c}}WRONG{{ELSE}}OK{{END}}

BODY;

$T = new Blitz();
$T->load($body);

$T->display(
	array(
		'a' => '123',
		'b' => '',
		'c' => 'textarea'
	)
);
?>
--EXPECT--
a: '123'
b: ''
expression: (a == 'text') || !b
result: OK

expression: (b == 'text') || !b
result: OK

c: 'textarea'
expression: (c == 'text') || !c
result: OK

expression: (c == 'text')
result: OK

expression: !c
result: OK
