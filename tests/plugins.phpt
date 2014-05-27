--TEST--
plugins
--FILE--
<?php
include('common.inc');

class Test {
    static function say($first, $second) {
        return 'first = "'.$first.'", second = "'.$second.'"';
    }
}

function say($s) {
    return $s;
}

$body = "New caller really works!
PHP function call: {{ strtolower('Your Revolution Is Over, Mr. Lebowski') }}
Simple user function call: {{ say(\$a); }}
Static user function call: {{ Test::say(\$a, \$b); }}
";

$T = new Blitz();
$T->load($body);
$T->display(array(
    'a' => 'fuck it, dude, let\'s go bowling',
    'b' => 'where is the money, lebowski?'
));

?>
--EXPECT--
New caller really works!
PHP function call: your revolution is over, mr. lebowski
Simple user function call: fuck it, dude, let's go bowling
Static user function call: first = "fuck it, dude, let's go bowling", second = "where is the money, lebowski?"

