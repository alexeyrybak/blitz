<?

$pwd = $_SERVER['PWD'];
if ($pwd[strlen($pwd)-1] !== '/')
    $pwd .= '/';
if (!preg_match('/tests\/$/', $pwd)) {
    $pwd = $pwd.'/tests/';
}

ini_set('blitz.path', $pwd);

?>
