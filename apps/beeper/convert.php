<?php
//
//
//    Copyright 2011 user890104
//
//
//    This file is part of emCORE.
//
//    emCORE is free software: you can redistribute it and/or
//    modify it under the terms of the GNU General Public License as
//    published by the Free Software Foundation, either version 2 of the
//    License, or (at your option) any later version.
//
//    emCORE is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//    See the GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License along
//    with emCORE.  If not, see <http://www.gnu.org/licenses/>.
//
//


if (
    empty($_SERVER['argc']) || empty($_SERVER['argv']) ||
    !is_int($_SERVER['argc']) || !is_array($_SERVER['argv']) ||
    3 !== $_SERVER['argc'] || 3 !== count($_SERVER['argv'])
)
{
    echo 'usage: ', $_SERVER['argv'][0], ' <input> <output>', "\n";
    exit;
}

$start_oct = 0;
$end_oct = 9;

$notes = array();

for ($oct = $start_oct; $oct < $end_oct; ++$oct)
{
    $notes[$oct] = array();
    $notes[$oct][-1] = 0;

    for ($note = 0; $note < 12; ++$note)
    {
        $freq = pow(2, ($oct * 12 + $note - 45) / 12) * 440;

        $notes[$oct][$note] = $freq;
    }
}

$note_names = array(
    'X' => -2,
    'P' => -1,
    'C' => 0,
    'C#' => 1,
    'D' => 2,
    'D#' => 3,
    'E' => 4,
    'F' => 5,
    'F#' => 6,
    'G' => 7,
    'G#' => 8,
    'A' => 9,
    'A#' => 10,
    'B' => 11,
);

// ----------------------

$beat_len = 0;

$note_lengths = array(
    '1+1+1/4' => 9,
    '1+1' => 8,
    '1' => 4,
    '2.5' => 3,
    '2+1/4' => 2.5,
    '2' => 2,
    '4.5' => 1.5,
    '4+1/2' => 1.5,
    '4' => 1,
    '8' => .5,
    '8.5' => .75,
    '8+1/2+1/4' => .3 + 5/4,
    '16' => .25,
    '32' => .125,
);

file_put_contents($_SERVER['argv'][2], '');

$file = file($_SERVER['argv'][1]);
$file = array_map('trim', $file);
$file = array_map('strtoupper', $file);

foreach ($file as $line)
{
    if (strlen($line) < 1 || '#' === $line[0])
    {
        continue;
    }
    elseif ('T' == $line[0])
    {
        $beat_len = 60 / trim(substr($line, 1));
        continue;
    }

    if (empty($beat_len))
    {
        continue;
    }
    $line = preg_replace('/\s+/', ' ', $line);
    @list($note, $oct, $len) = explode(' ', $line);

    $note = $note_names[$note];

    if (-2 === $note)
    {
        exit;
    }

    $len = $note_lengths[$len] * $beat_len;

    file_put_contents($_SERVER['argv'][2], pack('VV', round($notes[$oct][$note] ? 91225 / $notes[$oct][$note] : 0), round($len * 1000000)), FILE_APPEND);
}
?>
