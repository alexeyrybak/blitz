--TEST--
literal #1
--FILE--
<?php

include('common.inc');
ini_set('blitz.remove_spaces_around_context_tags', 1);

$body = <<<BODY
0
{{ LITERAL }}
1
{{ BEGIN }}
2
{{ END }}
3
BODY;

$T = new Blitz();
$T->load($body);
$T->display();

?>
--EXPECT--
0
1
{{ BEGIN }}
2
3
