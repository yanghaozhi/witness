<?php

$out_path = './';

switch ($argc)
{
default:
case 3:
    $out_path = $argv[2];
case 2:
    $db_path = $argv[1];
    if (file_exists($db_path) === false)
    {
        echo "can not find witness db : $db_path\n";
        exit (0);
    }
    break;
case 1:
    echo "Usage: php genhtml [db file] [html path]\n";
    exit (0);
}

if (is_dir("$out_path/src") === false)
{
    mkdir("$out_path/src", 0777, true);
}
if (is_dir("$out_path/var") === false)
{
    mkdir("$out_path/var", 0777, true);
}

copy('./glass.png', "$out_path/glass.png");
copy('./witness.css', "$out_path/witness.css");

$db = new SQLite3($db_path, SQLITE3_OPEN_READONLY);

$globals = array();
$results = $db->query('select * from globals;');
while ($row = $results->fetchArray())
{
    $globals[$row['name']] = $row['value'];
}

$global_urls = array();
$results = $db->query('select * from globals;');
while ($row = $results->fetchArray())
{
    $global_urls[$row['name']] = write_var($out_path, $row, 'g');
}

$error = false;

if (write_index($out_path) === false)
{
    echo "write index error\n";
}

echo "OK\n";

function write_index($path)
{
    global $db;

    $rows = array();
    $results = $db->query('select * from calls order by call desc;');
    while ($row = $results->fetchArray())
    {
        $func_id = $row['func'];
        $res = $db->querySingle("select * from funcs where func = $func_id;", true);
        $res['call'] = $row['call'];
        $res['prev'] = $row['prev'];
        $res['cur'] = $row['line'];

        $rows[] = write_src($path, $res);
    }

    global $globals;
    $params = array
    (
        'funcs' => ($globals['type'] == 'trace') ? array_reverse($rows) : $rows,
        'globals' => $globals,
    );

    return write_html($params, 'index.template.php', "$path/index.html");
}

function write_src($path, $info)
{
    if ((empty($info['file']) === true) || (file_exists($info['file']) === false))
    {
        return $info;
    }

    global $db;

    $vars = array();
    $results = $db->query('select * from locals where call = ' . $info['call']);
    while ($row = $results->fetchArray())
    {
        $vars[$row['name']] = write_var($path, $row, 'l');
    }

    $args = array();
    $results = $db->query('select n.name, v.value, c.call from args n, argvs v, funcs f, calls c where f.func = c.func and n.func = f.func and v.call = c.call and n.seq = v.seq and c.call = ' . $info['call']);
    while ($row = $results->fetchArray())
    {
        $args[$row['name']] = write_var($path, $row, 'a');
    }

    $fp = fopen($info['file'], 'r');

    $content = "<pre>\n";
    for ($i = 1; $i < $info['line_start']; ++$i)
    {
        $content .= write_line_no($i) . htmlspecialchars(fgets($fp));
    }

    for (; $i <= $info['line_end']; ++$i)
    {
        $content .= write_line_no($i) . write_line(fgets($fp), $i, $info, $vars, $args);
    }

    while (($tmp = fgets($fp)) !== false)
    {
        $content .= write_line_no($i++) . htmlspecialchars($tmp);
    }
    $content .="</pre>\n";

    fclose($fp);

    global $globals;
    $params = array
    (
        'globals' => $globals,
        'content' => $content,
    );

    $file = $info['call'] . '_' . basename($info['file']) . '.html';
    if (write_html($params, 'src.template.php', "$path/src/$file") === true)
    {
        $info['url'] = "src/$file#cur";
    }

    return $info;
}

function write_var($path, $info, $prefix)
{
    global $globals, $error;

    $error = false;
    $org = set_error_handler('witness_error_handler');
    $var = unserialize($info['value']);
    set_error_handler($org);

    $params = array
    (
        'name' => $info['name'],
        'value' => ($error === false) ? $var : $info['value'],
        'globals' => $globals,
    );

    $file = $prefix . '_' . ((isset($info['call']) === true) ? $info['call'] . '_' : '') . $info['name'] .'.html';
    if (write_html($params, 'var.template.php', "$path/var/$file") === false)
    {
        return null;
    }

    return write_link($file, $params['name'], $params['value']);
}

function write_link($file, $name, $value)
{
    $str = str_replace("\n", '&#10;', var_export($value, true));
    return "<a href=\"../var/$file\" title=\"$str\">$$name</a>";
}

function write_line($content, $line_no, $info, $locals, $args)
{
    global $global_urls;

    $patterns = array("/\n/");
    $replaces = array('');
    foreach ($global_urls as $k => $v)
    {
        if (is_null($v) === false)
        {
            $patterns[] = '/\$' . $k . '(\W)/';
            $replaces[] = $v . '\\1';
        }
    }

    $vars = ($line_no == $info['line_start']) ? $args : $locals;
    foreach ($vars as $k => $v)
    {
        if (is_null($v) === false)
        {
            $patterns[] = '/\$' . $k . '(\W)/';
            $replaces[] = $v . '\\1';
        }
    }

    global $db;
    $results = $db->query('select * from calls, funcs where funcs.func = calls.func and calls.prev =' . $info['call'] . ';');
    $cur = -1;
    while ($row = $results->fetchArray())
    {
        if ($row['line'] == $line_no)
        {
            $patterns[] = '/([^\$\w])' . $row['name'] . '(\s*\()/';
            $replaces[] = '\\1'. get_func_link($row['call'], $row['file'], $row['scope'] . '::' . $row['name'], $row['name']) . '\\2';
            $cur = $line_no;
        }
    }

    $content = preg_replace($patterns, $replaces, htmlspecialchars($content));

    switch ($line_no)
    {
    case $cur:
        return '<span class="lineNoCov">' . $content . "</span>\n";
    case $info['line_start']:
        global $db;
        $row = $db->querySingle('select * from calls c, funcs f where c.call = ' . $info['prev'] . ' and c.func = f.func;', true);
        if ((empty($row['scope']) === true) && (empty($row['name']) === true))
        {
            return '<span class="lineCov">' . $content . "    //no parent found : <a href=\"../index.html\">index page</a></span><a id=\"cur\"></a>\n";
        }
        else
        {
            return '<span class="lineCov">' . $content . '    //parent function : ' . get_func_link($row['call'], $row['file'], '', $row['scope'] . '::' . $row['name']) . "</span><a id=\"cur\"></a>\n";
        }
    default:
        return '<span class="lineCov">' . $content . "</span>\n";
    }
}

function get_func_link($call, $file, $title, $content)
{
    if ((empty($file) === true) || (file_exists($file) === false))
    {
        return $content;
    }
    else
    {
        return '<a href="./' . $call . '_' . basename($file) . '.html#cur"' . ((empty($title)) ? '' : " title=\"$title\"") . '</a>' . $content . '</a>';
    }
}

function write_line_no($line_no)
{
    return '<span class="lineNum"> ' . str_pad($line_no, 5, ' ', STR_PAD_LEFT) . ' </span>';
}

function write_html($params, $template, $file)
{
    ob_start();
    extract($params, EXTR_OVERWRITE);
    require __DIR__ .'/' . $template;
    $content = ob_get_clean();

    $fp = fopen($file, 'w');
    if ($fp === false)
    {
        echo "can not create file : $file\n\n";
        return false;
    }

    fwrite($fp, $content);

    fclose($fp);

    return true;
}

function witness_error_handler($errno, $errstr, $errfile, $errline)
{
    global $error;
    $error = true;
    return true;
}
