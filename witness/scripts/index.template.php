<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">

<html lang="en">

<head>
  <meta http-equiv="Content-Type" content="text/html; charset=ISO-8859-1">
  <title>WITNESS</title>
  <link rel="stylesheet" type="text/css" href="witness.css">
</head>

<body>

  <table width="100%" border=0 cellspacing=0 cellpadding=0>
    <tr><td class="title">witness report</td></tr>
    <tr><td class="ruler"><img src="glass.png" width=3 height=3 alt=""></td></tr>

    <tr>
      <td width="100%">
        <table cellpadding=1 border=0 width="100%">
          <tr>
            <td width="10%" class="headerItem">Current view:</td>
            <td width="35%" class="headerValue">call stack</td>
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
          <tr><td><img src="glass.png" width=3 height=3 alt=""></td></tr>
        </table>
      </td>
    </tr>

    <tr><td class="ruler"><img src="glass.png" width=3 height=3 alt=""></td></tr>
  </table>

  <center>
  <table width="80%" cellpadding=1 cellspacing=1 border=0>
    <tr><br></tr>
    <tr>
      <td width="05%" class="tableHead"> Call </td>
      <td width="20%" class="tableHead"> Function </td>
      <td width="35%" class="tableHead"> Scope/Class </td>
      <td width="40%" class="tableHead"> Source File </td>
    </tr>
    <?php foreach($funcs as $row){ ?>
    <tr>
      <td class="coverFile"><?php echo $row['call']; ?></td>
      <?php if (isset($row['url'])) {?>
      <td class="coverFile"><a href="<?php echo $row['url']; ?>"><?php echo $row['name']; ?></a></td>
      <?php } else { ?>
      <td class="coverFile"><?php echo $row['name']; ?></td>
      <?php } ?>
      <td class="coverFile"><?php echo $row['scope']; ?></td>
      <td class="coverFile"><?php echo $row['file']; ?></td>
    </tr>
    <?php } ?>
  </table>
  </center>
  <br>

  <table width="100%" border=0 cellspacing=0 cellpadding=0>
    <tr><td class="ruler"><img src="glass.png" width=3 height=3 alt=""></td></tr>
    <tr><td class="versionInfo">Author: <a href="mailto:yanghz@ucweb.com">yanghz@ucweb.com</a></td></tr>
  </table>
  <br>

</body>
</html>
