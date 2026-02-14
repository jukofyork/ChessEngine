2003_new_start: v1
==================
***> Have come back to the code after a long time (over a year?)... Bit lost :)
     * Deleted evaluation_old 1 and 2 (old versions - not used?).
     * Have delete all old evaluation sets (confused to what they are / do...)
     * Have altered lots of params in train_eval.cc, and intend long run.
     ? Have worked out the current eval is using backprop class (1 node!), and
       is non-symmetrical (ie: seperate You / Enemy feature values), and has
       a value for an empty square (ie: 13 peice_square features), and uses
       most of the 'standard' features also.
     ? It seems that the evaluation is in the range: -1 to +1, and there are
       no material evals being done (and no use of material values, as all is
       done with peice_square values by the NN - for training). It may be worth
       seeing if something can be done to avoid perminatly empty squares with
       no value (smooth with median, ect...).
     * Have set up train_over_time to work also!
     ??? CAN THE ROOK MOVE THROUGH 'CHECK' (IE:AN ATTACK?)...

2003_new_start: v2
==================
     * Have got rid of the BackProp class, and have gone back to using a normal 
       linear weighted sum (own code).
       - This has speeded up training from 3mins to 1:50sec...
     * Have made it so that the eval-set is scaled to make a pawn=1.0, and to 
       have all of the material values (means) subtracted for each peice type.
       - Have made a program (eval_train2play_util.cc) to do this.
       - This means we can use material evals again! (3x speedup!).
     NOTE: This MUST be run before playing/testing with eval-set!!!

2003_new_start: v3
==================
     ???? NO IDEA ????

2003_new_start: v4
==================
     * Have improved the convert_to_pgn.cc code and the SAN move stuff now:
       - Better parsing (less error prone).
       - Quick Quiescent search now has a 30 sec timeout ('silly' games...).
       - All *.min output is put on a single line. This is done to get rid of
         the need for a seperate program to randomize the data. Now use the
         program 'add_random_field.cc' along with sort to randomize.
         ALSO: The use of '| sort | uniq' is now possible to remove any twin
               games in the input data files.
     * Have removed the code that uses material evals (error prone...) and have
       got rid of the need to pass the eval set through another program after
       training (just use the output file, un-normalized, like before).
     * Have had to edit 'train_eval.cc' to NOT read past the eof (strange error
       in new c++?), and it first checks to see what size the file is before,
       and then uses this to seek back to the start (instead of re-open...).
     * When it finally completes their should be a very large database to train
       eval sets on (huge, millions of games).
     * Have now made it so that the program 'train_eval.cc' saves any important
       variables after training on each game. This means that it can be started
       and stopped freely (with sigTERM and sigINT) and it will not lose any of
       the work it has done. Also the output file can be appended to and it
       will look just the same, however many starts and stops have been done.
       This should work great for 'train_over_time' idea, and this means that
       training could be done over *many* weeks, by just logging into linux
       and leaving it running (without the hastle of exiting and losing work!).
       The precision for the eval set has also been upped to maximum, so as not
       to cause problems from the constant loading and saving of eval sets.
       NOTE: Make sure you use the same data file each time and also if it is
             mounted on window's C drive, make sure it it mounted in the
             train over time script...
     * Have now got the NN back for evaluation. It still uses 13 peice-sq inputs
       (ie: including empty squares as input) and each side (you and opponent) 
       have different weights. I have decided to keep this format so that it
       will be easy later to try having hidden layers and use sine activations 
       etc. The precision for the BacPropNet class has been upped for saving
       also (so as not to lose precision when loading and saving over and over).
       NOTE: Evaluation will be MUCH slower like this, but keep it so that
             proper NN can be used later (saves more messing)...
       Also, keep the output for the NN in the range {-1,1}, so that if an
       opening book is added later, we can easy calculate the values for each
       position to also be in the range {-1,1} (stats based on win/loss...)
       NOTE: Later it may be a good idea to make sure that any randomness
             (ie: RandomSwing), can't send the value above 1 or below -1???
     * Have now made the database file a binary format:
       2 bytes= game header (NumMoves/Result).
       3 bytes per move (same info as before, including quiescent status).
       This reduces the size of the data files by about a factor of 3, and was
       need because of the problem with the fact that there is a 2.1gb limit on
       file size. Both 'convert_to_pgn' and 'train_eval' work with this now, but
       it is no longer possible to use unix 'sort' and 'uniq' as before.
       - Use the program 'sort_lines' to do the same as '| sort | uniq' on the
         data file(s).
     * Have now fixed up the 'convert_from_pgn.cc' program to be much more
       reliable (see file for explantion...) and also the parsing of SAN moves
       in 'parse_pgn.cc' is much more reliable.
       NOTE: There is a problem of what to do with 'unterminated' comments
             or variations, but no easy fix - change later if current method
             leads to skipping large sections of data (ie: terminate on '[').
     * All the stuff that has been removed (programs, etc) - is all in the 
       folder programs/old_progs_etc.

2003_new_start: v5
==================
     * Converted code back to simple LWS code now (very fast).
       - Later make so the (PieceSquare) score is kept as a running total 
         (like material).
     * Have resurected the program to normalize the eval scores also now.
     * Have had to use a bipolar sigmoid activation on the outputs and use
       the 'generalized delta rule' for training, as the training did not work
       properly with linear activations (at all!).
     * Have tried to optimize the function 'ScoreMoves', by saving some of the
       heavily accessed structure elements in a static.

2003_new_start: v6
==================
     * Have deleted the old_progs drirrectory, use the stuff in v5 if you need 
       it again.
     * Have now resurected some old code from clean_up15b, whic has the 3 stages
       added into the features + lots of extra features to train.
       - Have placed the evaluation code and the old 'normalize_eval_set.cc', in
         the folder: 'old_stageless_features', so it should be easy to go back
         if it not work much better.
       - 'normalize_eval_set.cc', has been re-written and it not as hacked as
         before, as now it has two functions used to normalize and scale the
         values (see evaluation.cc).
       NOTE: The whole of the code for the evaluation has been coppied over,
             and ultimately it may be worth a complete re-write of all eval
             code (it's a nightmare!!!!) - watch for any possible bugs from
             old code creeping it (that were allready fixed in new ver!).
     * There was a problem with using the clock() function for timing...
       It was wrapping round every 72 minutes, which caused problems with the
       chess_test.cc program skipping stuff... Have now made a new function
       called Clock64(), which keeps track of wrap-arounds (every 72 mins), and
       returns a 64bit value.
       NOTE: This will go wrong if calls are not made for 72 minutes (no problem
             here).
       *NEXT* -> Implemet non-symetrical version... (uses twice as many weights)

2003_new_start: v7
==================
     * Have completely re-written the evaluation function, into a class.
       - It now uses stages and non-symetrical piece-square values, including
         evaluation of empty squares (add other features bit by bit now).
       - Now uses 3 macros which should make the eval code much easier to use
         and read:
         + AW(), is used to access a weight, and add the offset if we are
           training and doesn't if we are not training.
         + AWS(), is used to access a weight, and add the offset multiplied by
           a scaler value (for non-binary features such as distance...) if we 
           are training and doesn't if we are not training.
         + FIN(), flips the square's values, if the current player is black.
     * Have now made is do that the 'random swing', is added/taken from the
       whole of the evaluation set inside of think (using 'Mutate' function).
       - This should save on some CPU time, and means we no longer need to save
         and use it in the evaluation function...
     * Have now added back in a distance to own/other king feature for each
       piece in each stage (Not sure how good this is, used manhatton distance).
     * Have added in a-lot of king safety features (catling bonus, etc).
     * Have added back in the code to check for foreposts + extra code.
     * Have added back the code to check for open files for king, queen and rook
       and a test for open files to the left or right of the king.
     * Have added in the King/Queen X-Ray features back in now.
     * Have added back in the pawn-storm features, for a castled king.
     * Have added in feature(s) for battered/connected rooks and queens.
     * Have added in all the pawn features that were used before (from and
       older version, so has even more than was used before).

2003_new_start: v8
==================
     * Have now taken out any features that could be found using the 
       piece-square tables, such as king-in-centre files... This was because it
       could be causing problems with the normalizing etc.
     * Have now reduced the features, so that they are more general and do not
       matter if it is to the left or to the right or a certain square. This
       was done to try to improve the fact that the new features produced worse
       results than the old ones!!!
     * Have made it so that all features are active in all of the stages, this
       was done to try to improve the evauation also 9was like this before).
     * Have made it so that the opening uses 10+ peices rather than 12+ which
       it had been changed to.
     * Have increased the starting learning rate for train_eval to 0.0005, in 
       attempt to train faster (was 0.0001 before). This should be ok, as the
       LR-reduction is set to 0.75, meaning that it will quickly get low again.
     * Have got rid of the old dirrectory, see 'v7/programs/old' for a copy.
     *#* Not 100% sure if the features for an 'empty sqaure' and any of the
     *#* distance based fetures are actually helping (knight was in before?).
     *#* Hav tested the code, and it seems to run better (faster???), without 
     *#* these being used (i think...)???
         - Perhaps distances need be inverted (ie: subtracted from max?).
         - Perhaps we should use a bias to help distance (may cause probs with
           normalizing piece-square features?).
         - May turn out that only the distances are causing the probs, and not
           the empty square features...
           ... Something is causeing a bad drop in performance, eg:
               + larsen2.fin = 14/15 without both these, 6/7 with both!!!!
               + larsen2.fin (no king distance, but with empty squares) = 16!
               + larsen2.fin (with king distance, but no empty squares) = 10?
           ... Have decide to not-use king-based distance features, but use
               the empty squares (they were ok for the NN...) 
     * Have now decided to get rid of the rank battery and hav made it so that
       the file batery is faster. It now will give a double bonus if there are
       three majour peices on a single file and only checks 8 squares at most.
       - This was costing alot of cpu time, and the rank bonus was not working
         well - later have a bonus for having 2 or more mahour pieces on the
         7th rank.
     * Have now downloaed a new set of test positions, and it turns out that 
       the ?!/!? notation did mean that the move was bad (changed in new
       version of hardmid.fin)... Have changed chess_test to use these as bad 
       now.
       - Also there are two new test sets to use: 'dvor.fin' and 'krabbe.fin'.
         These have been included in the 'run_all_tests.sh' script now.
       - There may be other 'fixed' problems in the positions now, so don't
         worry too much if the results are differnt(ish) from before (hardmid!)
     *NEXT*: + Add text to the eval file to tell which feature is which...
             + Re-Write pawn features to be better, and also make the forepost
               check *not* use the PawnRank array (check for pawns...).
             + Add in bishop avoidance (bad bishop etc).
             + Two bishops feature.
             + Bishops of Opposite colour.
             + Bad trades.
             + E/D pawns getting blocked by minor pieces...
             + Rook behind past pawns.
             + Add back in a bonus for having two or more majour pieces on the
               7th rank (a bit like rank battery bonus from before).
             + Sort out the king xray (may be possible to make faster?).
             + Need to stop it putting bishops (and posibly knights on, e3
               and d3, when there is still a pawn behind (blocked centre).
             + Need to try to detect forposts that *could* be occupied, but
               are not being (yet), this is a common problems.
             ? Consider writting two evaluation functions for each type of
               piece (eg: EvalPawnWhite, EvalKingBlack). This could posssibly
               make the code *much* faster, at the expense of hastle writting
               two of each bit of code...
             ? Somehow there needs to be a balance between the cost in CPU of a 
               feature and it's usefulness. This may mean that the idea of a
               EP-GA, using some king of genetic programming language may be
               one good way to find useful features.
               ie: Fitness = 'Time Taken'/'Correct Predition' (or something like
                                                               this!).
             ? Or a way to test the result (ie: benifit/loss) of removing a
               feature (ie: greedy algorithm...).

2008: Chess
===========
1. Have come back to this after nearly 5 years.
2. Have decided to only consider positions which have equal material evaluations for training.
3. Have set the linear training target to 1.5 pawns as 1.5 pawns is supposed to be the critical value.
4. Have divided the trained eval by 3 so as to make it as thought we had trained with a target of 0.5 pawns.
5. Have reduced the eval window back down to 0.5 again.
6. Have reduced the apsiration window down to 0.25 from 0.5 now.
7. Have made it so that we only allow evaluations in graduations of 1/50th of a pawn to help speed search.
