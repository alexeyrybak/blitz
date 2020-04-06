--TEST--
rawurlencode output wrapper
--FILE--
<?php
include('common.inc');

$T = new Blitz();
$T->load('{{ rawurlencode($a); }}
{{ rawurlencode($b) }}
');

$T->set(
    array(
        'a' => 'foo @+%/',
        'b' => 'name#tag',
    )
);

echo $T->parse();

?>
--EXPECT--
foo%20%40%2B%25%2F
name%23tag
