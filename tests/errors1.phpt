--TEST--
errors and warnings: syntax
--FILE--
<?php
include('common.inc');
error_reporting(E_ALL);
set_error_handler('my_error_handler');
function my_error_handler($errno, $errstr, $errfile, $errline) {
    $parts = explode('ERROR:',$errstr);
    if (preg_match('/^(.+?) \(.+?:( line \d+, pos \d+)/',$parts[1], $matches)) {
        echo trim($matches[1].' (errors.tpl:'.$matches[2]).")\n";
    }
}

// show all syntax error warnings, that can be thrown by blitz
$T = new Blitz('errors.tpl');

?>

--EXPECT--
invalid method call (errors.tpl: line 2, pos 5)
invalid method call (errors.tpl: line 4, pos 1)
invalid method call (errors.tpl: line 5, pos 3)
unexpected TAG_CLOSE (errors.tpl: line 7, pos 1)
unexpected TAG_OPEN (errors.tpl: line 8, pos 1)
invalid method call (errors.tpl: line 10, pos 5)
end with no begin (errors.tpl: line 11, pos 3)
invalid <if> syntax, only 2 or 3 arguments allowed (errors.tpl: line 12, pos 1)
invalid <inlcude> syntax, only 1 argument allowed (errors.tpl: line 13, pos 3)
invalid method call (errors.tpl: line 15, pos 5)
