--TEST--
complex conditions sanity
--FILE--
<?php
include('common.inc');

error_reporting(E_ALL);
set_error_handler('my_error_handler');
function my_error_handler($errno, $errstr, $errfile, $errline) {
    $parts = explode('ERROR:',$errstr);
    if (preg_match('/^(.+?) \(.+?:\s*(line \d+, pos \d+)/',$parts[1], $matches)) {
        echo trim($matches[1].' ('.$matches[2]).")\n";
    }
}

$body = <<<BODY
{{ IF i) }}
{{ IF i)) }}
{{ IF (i)) }}
{{ IF (i }}
{{ IF ((i }}
{{ IF && i }}
{{ IF () }}
{{ IF () > 5 }}
{{ IF ()bla }}
{{ IF }}
{{ IF a||b||c||d||e||f||g||h||i||j||k||l||m||n||o||p||q||r||s||t||u||v||w||x||y||z||0||1||2||3||4||5||6||7||8||9 }}
{{ IF a || b || c || d || e || f || g || h || i || j || k || l || m || n || o || p || q || r || s || t || u || v || w || x || y || z || 0 || 1 || 2 || 3 || 4 || 5 || 6 || 7 || 8 || 9 }}
{{ IF a b c d e f g h i j k l m n o p q r s t u v w x y z 0 1 2 3 4 5 6 7 8 9 }}
BODY;

$T = new Blitz();
$T->load($body);

echo "####\n";

$body = <<<BODY
{{ IF && i }} i {{ END }}
{{ IF ! }} i {{ END }}
{{ IF i || }} i {{ END }}
BODY;

$T = new Blitz();
$T->load($body);
$T->display(array('i' => false));

?>
--EXPECT--
invalid <if> syntax, bracket mismatch (line 1, pos 1)
invalid <if> syntax, bracket mismatch (line 2, pos 1)
invalid <if> syntax, bracket mismatch (line 3, pos 1)
invalid <if> syntax, bracket mismatch (line 4, pos 1)
invalid <if> syntax, bracket mismatch (line 5, pos 1)
invalid <if> syntax, wrong number of operands (line 6, pos 1)
invalid <if> syntax, empty expression between the brackets (line 7, pos 1)
invalid <if> syntax, empty expression between the brackets (line 8, pos 1)
invalid <if> syntax, empty expression between the brackets (line 9, pos 1)
invalid <if> syntax, probably a bracket mismatch but could be wrong operands too (line 10, pos 1)
invalid <if> syntax, the expression is too complex, consider raising BLITZ_IF_STACK_MAX=32 (line 11, pos 1)
invalid <if> syntax, the expression is too complex, consider raising BLITZ_IF_STACK_MAX=32 (line 12, pos 1)
invalid <if> syntax, the expression is too complex, consider raising BLITZ_IF_STACK_MAX=32 (line 13, pos 1)
####
invalid <if> syntax, wrong number of operands (line 1, pos 1)
invalid <if> syntax, wrong number of operands (line 2, pos 1)
invalid <if> syntax, wrong number of operands (line 3, pos 1)
