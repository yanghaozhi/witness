<?php
include "base/Loader.php";

$c = "Index";
$a = "display";

$controlName = $c."Controller";
$actionName = $a."Action";

$obj = new $controlName();
$obj->$actionName();

