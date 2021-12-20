# Othello_GeneticAlgorithm

１回生秋学期の期末課題「なるべく強いオセロをつくる」で作成した、遺伝的アルゴリズムを実装したオセロのプログラム。適当に打った教授に61-2で勝った。

## 仕組み
・次の手の探索にはαβ法を用い、遺伝的アルゴリズムで重みづけした評価関数を利用した。

・評価関数では、四隅～辺の確定石の数と、その盤面における石配置の自由度で評価値を計算している。
