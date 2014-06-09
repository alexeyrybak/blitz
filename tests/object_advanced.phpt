--TEST--
object advanced (not expecting just arrays)
--FILE--
<?php
include('common.inc');

if (function_exists('date_default_timezone_set'))
    date_default_timezone_set('UTC');

ini_set('date.timezone', 'UTC');

$body = <<<BODY
{{ BEGIN users }}
Name: {{ IF(name, name, "Unnamed") }}
Age: {{ age(age); }}
Birthdate: {{ date("%Y/%m/%d", \$age); }}
{{ include(tpl); }}
{{ END users }}
===================================================================

BODY;

// Passing an object to a function resulted in a segfault in 0.8.9
function age($s) {
    list($y, $m, $d) = explode('-', $s->birthdate, 3);
    $alreadyPassed = (sprintf('%02d%02d', $m, $d) > date('dm') ? 0 : 1);
    return date('Y') - $y + $alreadyPassed;
}


class Age {
    public $birthdate;

    public function __construct($birthdate) {
        $this->birthdate = $birthdate;
    }

    public function __toString() {
	// Ugly hack to have the object returned an integer, so date() can play with it
        return "" . strtotime($this->birthdate);
    }
}

class Tpl {
    public function __toString() {
        return 'include-inner.tpl';
    }
}

class User {
    public $name;
    public $age;
    public $a;
    public $b;
    public $tpl;

    public function __construct($name, $birthdate) {
        $this->name = $name;
        $this->age = new Age($birthdate);
        $this->tpl = new Tpl();
        $this->a = 'a';
        $this->b = 'b';
    }
}

$vincent = new User("Vincent", "1982-11-01");
$maurus = new User("Maurus", "1991-04-07");
$unknown = new User(false, "1987-08-14");

$T = new Blitz();
$T->load($body);
$T->display(array('users' => array($vincent, $maurus,$unknown)));

?>
--EXPECTF--
Name: Vincent
Age: %d
Birthdate: 1982/11/01
a = a, b = b


Name: Maurus
Age: %d
Birthdate: 1991/04/07
a = a, b = b


Name: Unnamed
Age: %d
Birthdate: 1987/08/14
a = a, b = b


===================================================================
