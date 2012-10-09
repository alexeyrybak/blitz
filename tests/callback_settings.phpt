--TEST--
callback settings
--FILE--
<?php

include('common.inc');

function m() {
    return "PHP function m() was called";
}

class Helper {
    static function m() {
        return "Helper function m() was called";
    }
}

class View extends Blitz {
    function m() {
        return 'View function m() was called';
    }
}

$T = new View();

$T->load(
"{{ m(); }}
{{ this::m(); }}
{{ Helper::m(); }}
{{ php::m() }}
");

$ini = array(
    0 => array(
        'blitz.enable_callbacks' => 0,
        'blitz.enable_php_callbacks' => 1,
        'blitz.php_callbacks_first' => 1,
    ),    
    1 => array(
        'blitz.enable_callbacks' => 1,
        'blitz.enable_php_callbacks' => 1,
        'blitz.php_callbacks_first' => 1,
    ),
    2 => array(
        'blitz.enable_callbacks' => 1,
        'blitz.enable_php_callbacks' => 0,
        'blitz.php_callbacks_first' => 1,
    ),
    3 => array(
        'blitz.enable_callbacks' => 1,
        'blitz.enable_php_callbacks' => 1,
        'blitz.php_callbacks_first' => 0,
    ),
    4 => array(
        'blitz.enable_callbacks' => 1,
        'blitz.enable_php_callbacks' => 0,
        'blitz.php_callbacks_first' => 0,
    ),
);

$i = 0;
foreach ($ini as $data) {
    echo 'TEST #'.$i."\n";
    if (!empty($data)) {
        foreach ($data as $key => $value) {
            ini_set($key, $value);
        }
    }

    $T->display();
    $T->clean();
    $i++;
}

?>
--EXPECT--
TEST #0
blitz::display(): callbacks are disabled by blitz.enable_callbacks, m call was ignored, line 1, pos 1
blitz::display(): callbacks are disabled by blitz.enable_callbacks, m call was ignored, line 2, pos 1
blitz::display(): callbacks are disabled by blitz.enable_callbacks, helper::m call was ignored, line 3, pos 1
blitz::display(): callbacks are disabled by blitz.enable_callbacks, m call was ignored, line 4, pos 1




TEST #1
PHP function m() was called
View function m() was called
Helper function m() was called
PHP function m() was called
TEST #2
blitz::display(): PHP callbacks are disabled by blitz.enable_php_callbacks, helper::m call was ignored, line 3, pos 1
blitz::display(): PHP callbacks are disabled by blitz.enable_php_callbacks, m call was ignored, line 4, pos 1
View function m() was called
View function m() was called


TEST #3
View function m() was called
View function m() was called
Helper function m() was called
PHP function m() was called
TEST #4
blitz::display(): PHP callbacks are disabled by blitz.enable_php_callbacks, helper::m call was ignored, line 3, pos 1
blitz::display(): PHP callbacks are disabled by blitz.enable_php_callbacks, m call was ignored, line 4, pos 1
View function m() was called
View function m() was called


