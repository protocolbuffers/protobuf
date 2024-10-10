Protocol Buffers - 谷歌的数据交换格式
==================================

[![OpenSSF Scorecard](https://api.securityscorecards.dev/projects/github.com/protocolbuffers/protobuf/badge)](https://securityscorecards.dev/viewer/?uri=github.com/protocolbuffers/protobuf)

版权所有 2023 Google LLC

概述
----

Protocol Buffers（简称 protobuf）是谷歌开发的一种与语言和平台无关、可扩展的机制，用于序列化结构化数据。你可以在 [protobuf 的文档](https://protobuf.dev) 中了解更多信息。

本 README 文件包含 protobuf 的安装说明。要安装 protobuf，你需要安装协议编译器（用于编译 .proto 文件）以及为你选择的编程语言提供的 protobuf 运行时。

使用 Protobuf 源代码
----------------------

大多数用户会发现从[支持的发布版本](https://github.com/protocolbuffers/protobuf/releases)开始使用是最简单的方式。

如果你选择从主分支的最新版本开始工作，偶尔会遇到由于源代码不兼容的更改和未经充分测试（因此存在问题）的行为而导致的构建失败。

如果你使用 C++ 或者需要将 protobuf 源代码作为你项目的一部分进行构建，应该固定使用发布分支上的发布提交版本。

这是因为即使是发布分支，在发布提交之间也可能会出现一些不稳定的情况。

Protobuf 编译器安装
-------------------

protobuf 编译器是用 C++ 编写的。如果你使用 C++，请按照 [C++ 安装说明](src/README.md) 安装 protoc 以及 C++ 运行时。

对于非 C++ 用户，最简单的安装协议编译器的方式是从我们的 [GitHub 发布页面](https://github.com/protocolbuffers/protobuf/releases) 下载预构建的二进制文件。

在每个版本的下载部分，你可以找到以 zip 包形式提供的预构建二进制文件：`protoc-$VERSION-$PLATFORM.zip`。其中包含 protoc 二进制文件以及随 protobuf 一起分发的一组标准 `.proto` 文件。

如果你需要查找发布页面上没有的旧版本，请查看 [Maven 仓库](https://repo1.maven.org/maven2/com/google/protobuf/protoc/)。

这些预构建的二进制文件仅为发布版本提供。如果你想使用 GitHub 主分支的最新版本，或需要修改 protobuf 代码，或者你正在使用 C++，建议从源代码构建自己的 protoc 二进制文件。

如果你想从源代码构建 protoc 二进制文件，请参阅 [C++ 安装说明](src/README.md)。

Protobuf 运行时安装
-------------------

Protobuf 支持多种不同的编程语言。对于每种编程语言，你可以在相应的源代码目录中找到如何为该特定语言安装 protobuf 运行时的说明：

| 编程语言                             | 源代码目录                                                    |
|--------------------------------------|-------------------------------------------------------------|
| C++（包括 C++ 运行时和 protoc）      | [src](src)                                                  |
| Java                                 | [java](java)                                                |
| Python                               | [python](python)                                            |
| Objective-C                          | [objectivec](objectivec)                                    |
| C#                                   | [csharp](csharp)                                            |
| Ruby                                 | [ruby](ruby)                                                |
| Go                                   | [protocolbuffers/protobuf-go](https://github.com/protocolbuffers/protobuf-go)|
| PHP                                  | [php](php)                                                  |
| Dart                                 | [dart-lang/protobuf](https://github.com/dart-lang/protobuf) |
| JavaScript                           | [protocolbuffers/protobuf-javascript](https://github.com/protocolbuffers/protobuf-javascript)|

快速开始
--------

学习如何使用 protobuf 的最佳方式是按照我们[开发者指南中的教程](https://protobuf.dev/getting-started)。

如果你想从代码示例中学习，可以查看 [examples](examples) 目录中的示例。

文档
----

完整的文档可以在 [Protocol Buffers 文档站点](https://protobuf.dev) 上找到。

支持政策
--------

阅读我们的[版本支持政策](https://protobuf.dev/version-support/)以了解各语言库的支持时限。

开发者社区
----------

如果你想了解 Protocol Buffers 的最新动态并与 protobuf 的开发者和用户进行交流，可以[加入 Google 讨论组](https://groups.google.com/g/protobuf)。