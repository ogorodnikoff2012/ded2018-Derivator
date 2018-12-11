#pragma once

static constexpr char kTexPreamble[] = R"(
\documentclass{article}
\usepackage[a4paper,margin=2cm]{geometry}
\usepackage[utf8]{inputenc}
\usepackage[T2A]{fontenc}
\usepackage[russian]{babel}
\usepackage{amsthm}
\usepackage{amssymb}
\usepackage{amsopn}
\usepackage{amsmath}
\usepackage{indentfirst}
\usepackage{amsfonts}
\usepackage{paralist}

\title{Домашнее задание по математическому анализу}
\author{Владимир Огородников}
\date{}

\begin{document}
    \maketitle
    Упростить следующие выражения:
)";

static constexpr char kTexEnd[] = R"(
\end{document}
)";

static constexpr char kTexError[] = R"(
    Упс, что-то пошло не так...
)";
