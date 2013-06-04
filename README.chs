================================================================
 witness: 用于跟踪/调试/监控 PHP 运行的扩展.
 版权所有 (c) 2013 UCWeb Inc.
================================================================

witness 在 Apache License version 2 开源协议下发布，
具体内容（英文）请参见 LICENSE 文件

包结构

  README.en      - 英文版的README
  README.chs     - 本文档
  LICENSE        - 开源协议
  ./witness      - 源码目录
  ./include      - 包含文件目录

介绍

1. 环境依赖
  操作系统       - Linux/*nix (i686/x86_64/amd64)
  gcc 版本       - >= 4.1.2
  PHP 版本       - >= 5.3.3

2. 编译和安装
  $ cd witness
  $ phpize && ./configure && make && make install

3. php.ini
  在 php.ini 文件中添加以下内容 :
  [xhprof]
  extension=witness.so

4. 检查
  $ php -m | grep witness
  witness

5. 查看信息
  $ php -i | grep witness
  witness
  witness support => enabled
  witness.auto_start_token => no value => no value
  witness.enable_assert => false => false
  witness.enable_dump => 1 => 1
  witness.keep_record => false => false
  witness.max_stack => 16384 => 16384
  witness.post_url => no value => no value
  witness.record_internal => 1 => 1
  witness.temp_path => /dev/shm => /dev/shm

如有任何问题，请联系 oswitness@ucweb.com

谢谢

== END OF FILE ==
