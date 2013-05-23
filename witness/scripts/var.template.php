<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">

<html lang="en">

<head>
  <meta http-equiv="Content-Type" content="text/html; charset=ISO-8859-1">
  <title>WITNESS</title>
  <link rel="stylesheet" type="text/css" href="../witness.css">
</head>

<body>

  <table width="100%" border=0 cellspacing=0 cellpadding=0>
    <tr><td class="title">witness report</td></tr>
    <tr><td class="ruler"><img src="../glass.png" width=3 height=3 alt=""></td></tr>

    <tr>
      <td width="100%">
        <table cellpadding=1 border=0 width="100%">
          <tr>
            <td width="10%" class="headerItem">Current view:</td>
            <td width="35%" class="headerValue">Variable's Value</td>
          </tr>
          <tr>
            <td class="headerItem">Token:</td>
            <td class="headerValue"><?php echo $globals['token']; ?></td>
            <td></td>
          </tr>
          <tr>
            <td class="headerItem">Start:</td>
            <td class="headerValue"><?php echo $globals['start']; ?></td>
            <td></td>
          </tr>
          <tr>
            <td class="headerItem">End:</td>
            <td class="headerValue"><?php echo $globals['end']; ?></td>
            <td></td>
          </tr>
          <tr><td><img src="../glass.png" width=3 height=3 alt=""></td></tr>
        </table>
      </td>
    </tr>

    <tr><td class="ruler"><img src="../glass.png" width=3 height=3 alt=""></td></tr>
  </table>
  <pre>
    <?php echo "\nvar_dump($$name)\n"; var_dump($value); ?>
  </pre>

  <table width="100%" border=0 cellspacing=0 cellpadding=0>
    <tr><td class="ruler"><img src="../glass.png" width=3 height=3 alt=""></td></tr>
  </table>

  <pre>
    <?php echo "\nprint_r($$name)\n"; print_r($value); ?>
  </pre>
  <br>

  <table width="100%" border=0 cellspacing=0 cellpadding=0>
    <tr><td class="ruler"><img src="../glass.png" width=3 height=3 alt=""></td></tr>
    <tr><td class="versionInfo">Author: <a href="mailto:yanghz@ucweb.com">yanghz@ucweb.com</a></td></tr>
  </table>
  <br>

</body>
</html>
