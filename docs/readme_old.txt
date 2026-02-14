// clean_up19 Sat Mar  9 14:24:52 GMT 2002
// =======================================
// V0: This was originally: "chess/eval_learning/chess+eval_class4.cc".
// It was converted back to just be a chess player and not an evaluation
// learner because it has has a lot of the bugs fixed in it (see old prog cmts).
// It should work the same as before, but with the odd bug removed.
// Have also removed the headers 'math.h' and 'string.h' as they were not
// needed.

// V1/V2 are crap versions with bad eval() tables.

// V3: This version has a slightly altered pawn table with only -30 (Not -40)
//     for a pawn on Queen2 or King2.
//     This means that King's Pawn/Queen's Pawn and Knight openings are
//     all cheanged frequently.

// V4: This version now has a new PrintBoard() function that will print the
//     board the other way up for a human player if he plays black.
//     This is to make it easier to see what is going on when playing
//     on another (real) chess board.
//     Also fixed a small bug with the king (Open/Middle) table that
//     had the king's start square -ve.

// V5: Have got rid of all switch statments and made all into if/else 
//     statements.
//     Have also fixed a bug (and altered to have semi-backward) in the 
//     backward pawn evaluation that made black do worse the further up
//     the baord he was (bad!).
//     Have added Eval() code for knight/bishop on forepost's aswell.

// V6: Have now made the King Scaler Maximum a define.
//     Have also alterd some of the eval values to make less valuble.
//     Made the PAWN_STORM value addative (not exponential).
//     Have made the square in front of the king have a double penalty
//     if it has a empty/half empty file in front of it.
//     ALso fixed bug in the orientation.

// V7: Have now added a timer limit to the Think() function. It will
//     allow a search (at depth N) to continue until the time is over, this 
//     also means that the search will usually last longer than the time
//     per move (even up to 10 time sometimes), but does play a better end
//     game.
//     Have also set back the peice values to 1,3,3.5,5,9 as the others from
//     Holyes book were makeing a swap for Rook+Pawn/Minor+Minor which
//     does not seem sensible.
//     Also useing the Normal (Full) Repitition Check now.

// V8: Now uses the clock() function rater than the time function to get
//     a better time limit than to the nearest second.
//     Have also fixed the problem with the printing of the move type in the PV,
//     caused by using N&X==TRUE, when really should be N&X>0...

// V9: Have now made the computer NOT think when it has only one choice,
//     e.g. a forced move is done instantly (for the computer only!).

// V10: Have removed the QUICK_REP_CHECK as it is useless.
//      Have Made the PositionHistory Vector hold the whole game state, so
//      the user may take back a move by using the 't' key which calls the
//      UserTakeMoveBack() function.
//      Have also fixed a bug in the was the game state was stored in the fact
//      that the Rook's move in a castle move was updated before the state was
//      saved. Fixed by doing right at the beginning of the MakMove function.
//      Have also now stopped the computer from thinking, when he knows for
//      definite he is going to mate or get mated.

// V11: Have now made the Timeing Out work at any time. It will quit the
//      search and do the best move on the last deepest search.
//      Have made the notification of mate work on quintessance moves and also
//      the cuttof works for quintessance moves now, so no more search will be 
//      done if a quintessance mate is found.
//      Have now made the search time floating point to have fractions of time.
//      *> One problem is the fact that if we only have one option to take, then
//      *> the number of moves until mate is not reported because Think() is not
//      *> called for moves with only one choice and hence no score is found for
//      *> them... Not a big problem, but eventually needs sorting out!

// V12: Have added the insuficient material draw function to the search and
//      the result printer.
//      Can't test yet as will not do it now!

// no_class: Have now split the whole program back into bits, with no Chess
//           class used anymore.
//           All seems to work OK - Much better like this!
//           Have also tested the Material Mate and it seems to work.

// no_class2: Have now speeded up the TestForRepitition() and MakeMove() 
//            functions by using memcmp() and memcpy() functions to do the work
//            rather than using native C/C++ loops.
//            This has made the whole program go about twice a s fast as before
//            and the Eval() function is now the most time consuming using 
//            around 25% of CPU.
//            Actually make very little differnce when optimized, may even make
//            slightly slower! But when tested with one that has re_check 
//            disabled it makes not a lot of differnce!

// no_class3: Have now made the evaulation function a bit better by doing the 
//            following things:
//            - Made extra masks for Rook and Queen.
//            - Now for each Piece (other than King) we have an opening mask
//              and a middle/end game mask.
//            - Queen is penalised for moving in the opening now.
//            - Rook is penalized for moving Up the board in the opening.
//            - Knight is worth +50ish in the open and bishop is worth +50 in 
//              the end game (middle keeps both the same with no bonus).
//            - Several of the tables have been altered to do a better 
//              selection of openings and other positional advantages.

// no_class4: Can't realy remember???
//            - Some things put back to the way they were.

// no_class5: Have made extra forepost bonuses for a pawn protexted forepost
//            and also for an absolute forepost.

// no_class6: Have now made all options taken on the command line to simplify 
//            the interface ready for later options to be added.
//            Have made random swing settable from the command line.
//            Have also made the Win Score 100000 now ready for later.
//            Have now made the evaluation paramter set loadable from file,
//            this means that a set may be loaded for both computer players
//            white or black and used in the eval function.
//            Found a bug in that there were only 7 columns in one of the queen
//            and rook value masks.
//            Should now be able to run a GA to find good parameter sets by
//            doing the following (by using system function):
//            - Convert to a eval data file for each.
//            - Run program with the output off with these data files.
//            - Read the result and score fitness based on this.
//            -- Will need the folowing ranges:
//            #define MAX_END_GAME_VAL    4000
//            #define MAX_KING_SCALE      10
//            #define MAX_BON_PEN_VAL     200
//            #define MAX_PIECE_VAL       2000     // Pawn may be set to 100!
//            #define MAX_MIN_MASK_VAL    200
//            #define MIN_END_GAME_VAL    100
//            #define MIN_KING_SCALE      1
//            #define MIN_BON_PEN_VAL     0
//            #define MIN_PIECE_VAL       100

// no_class7: Have now added back the rook on seventh bonus.
//            Have reduced the tables in learn_eval.c to 109 from 794.
//            STILL NEED TO ADDD SOEMTHING TO ENCORAGE CASTLEING.

// no_class8: Have now made the GA part of the program.
//            Still need to find a better way of running an evolutionary
//            tournament. Not even sure if useing search depth of 1 means
//            anything. Never seems to have a converged population or even find
//            anything any good.

// no_class9: * Come back to this after about a year, so still rusty etc... *
//            Have fixed a bug in the Evolve function that meant it was not
//            working at all (always same player playing self!).
//            Have altered several parameters of the GA to test with.

// no_class10: Have found a function to replace the FLIPPER[Square] vector.
//             Have also altered the Evolve() function and Genome class.

// start_again1: These versions are simplifing versions.
//               * Have now removed the (crappy, do better later) eval learning.
//               * Have made it so TakeMoveBack simply copies from the state
//                 history rather than saving a sparse rep, this is much simpler
//                 to understand and only slows prog down by about 1% to 2%.
//                 Also, there is no more UserTakeMoveBack.
//               * Have got rid of the following functions:
//                 - GenPromote() is now is GenPush.
//                 - GenCaptures() now just used GenMoves and tests the move
//                   type in the Quiesce search (1% to 2% speed reduction only).
//               * Have now got rid of the idea of removing the repitition
//                 check, as must realy have this for chess.
//               * Have now created a very simple (but usable) interface, a bit
//                 botched at the mo - but has potential.
//                 Don't bother to improve it too much, until both this and the
//                 canvas class hit stable versions.
//               * Have now got rid of the GenRecord structure. This was done by
//                 simply adding the score to the MoveStruct structure. 
//                 Suprisingly this actually speeds the code up by 1% to 2%.
//                 BEWARE: Dirrect comparison of moves now must ignore this, by
//                         doing something like sizeof(MS)-sizeof(int) as in the
//                         SortPV() function.

// start_again2: * Have now got rid of the 1D GenData move lists and have now 
//                 made it into a 2D (ie: [Ply][MoveIndex]) move list.
//                 As before, interface only uses the first ply (ie: [0][?]).
//               * Have also altered a few of the constant maximuns, that should
//                 realy be dynamic vextors ect, to names that better reflect
//                 there meaning.
//               * Have now turned the Piece[] and Colour[] vectors back to
//                 using charsi/bytes (rather than 32bit ints). Also using
//                 memcpy rather than own crap copy for loop, made the program
//                 go 33% faster!!!
//               * Have now speeded up the TestRepition() function slightly
//                 by taking into account the fact that the Fifty-Move counter
//                 tells us which N (up to 50) states to look back at, rather
//                 than looking at all previous states (0% speed inc so far?).
//                 Also the fact that we must have 8 moves since the Fifty-Move
//                 counter was set, rather than from game start, helps to speed
//                 the program up much more (29% speed increase - see bellow!).
//                 Ran profile after this, removing 'mcount' time factor now:
//                   => Standard test (4ply) took 1 minute 16 seconds.
//                 - Evaluation (main function)   : 41% of CPU.
//                 - GenMoves                     : 11% of CPU.
//                 - GenPush                      : 9%  of CPU.
//                 - Attack                       : 8% of CPU.
//                 - EvalPawn                     : 7% of CPU.
//                 - EvalKing                     : 4.5% of CPU.
//                 - MakeMove                     : 4% of CPU.
//                 - QuiesceSearch                : 4% of CPU.
//                 - Sort                         : 3% of CPU.
//                 NOTE: 99% of CPU is taken from the above functions only.
//                       Evaluation (all 3 functions) takes up 52% of CPU alone.
//                       User interface is totally insignificant as used less 
//                       than 0.01 seconds for a whole 12 hour run!
//                       With the above code in place TestRepitition() function
//                       takes only 0.27% of CPU, when before it took 22%.
//                       It should be noted that the 5 min per move profile
//                       had TestRepitition taking only 8.5% of CPU and times
//                       looked pretty much as they do above, even so this is
//                       still a good reduction, even for 5 min per move test.
//               * Have hacked in a beta window, sort out properly tommorow!  
//                 NOTE: There may be an even better way of doing this!


// start_again3: * Have now sorted out the above beta window hack to keep a 
//                 running total of material and insist that no more than 1 pawn
//                 worth difference from material is allowed through bonuses
//                 etc. This works well and reduces calls to the Eval() function
//                 by a factor of about 1.5 to 2.
//                 LATER: Sort out Evaluation to use smaller bonuses/penaltys,
//                        at the moment the old bonus/pen score is being divided
//                        by 4 and then normalized (bad!).
//               * Have also discoved that increasing the movehistory for beta
//                 cuttoffs by far more than alpha updates. This is only small
//                 speed increase, but does help slighly.
//               * Have made the peice values constant again (for quick eval).
//               * Have made the current state now live on the GameHistory.
//                 This caused a small amount of slowdown, until pointers
//                 were used for the current state in the history, then the
//                 speed was the same, but code is clearer with one less set of
//                 globals. May be sinsible in the future to try to speed this
//                 up with more pointers (to Colour[]/Piece[] ect).
//                 Also MakeMove() and TakeMoveBack() are simpler from useing
//                 the game history for hgolding the current state.
//               * Have now got the alpha window working by thinking like this:
//                            ...-----W****A***...***B****W-----...
//                               NO!^ <-------WORK!-------> ^NO!
//                 As Alpha<(=) Beta always!
//                 This with the beta window make a 50% speed increase!!!
//                 Calls to eval: Before=1306972 / After=357767 (3.6x less!).
//               * Have sorted out the setting of MoveHistory to use a shift
//                 to the right of the depth, rather than adding the depth.
//                 This ensures that the highest depth cuttoffs are tried first.
//               NOTE: Have discovered that because of the way Sort() selects
//                     things and the fact that we no longer have a GenCapt()
//                     fucntion, CAPTURES and PROMOTIONS must be ranked highest.
//                     (other than the PV of course).
//               * Have increased the Scoreing offsets (PV,CAPT,PROM) to larger
//                 values.

// start_again4: * Have now got the null move heuristic working.
//                 Not 100% sure if it's working correctly, but now some cludges
//                 have been fixed, it appears ok (5 games/5 against old 
//                 version looked ok, with 4 wins and a draw to null searcher!).
//               * There were some weird bugs when played agains gnuchess, but
//                 I think this was because of the DEEP-NULL idea not working.
//                 Also note the depth is set to 0 rather than -3 as in gnuchess
//                 which may NOT be ok - seems ok so far.
//                 Otherwise it has improved the speed of search by about 2 to
//                 3 extra levels on average!

// start_again5: * Have now implemented the 'Fail-Soft' alpha/beta prunning
//                 algorithm - taken from gnuchess 5.0 source.
//                 I seems to be working correctly, except one small thing:
//                 - When tested on 4 deep against start_again4 (no null!), at
//                   move 7 (depth 4) the old program sees only a -5, but this
//                   version sees a -3 (but prunes a -5????) - it may be a bug
//                   in either version and am still not sure. The new versions
//                   move seems more sensible, but who knows - look at SOON!
//                 Also, in order to get it cludged in I have no longer got
//                 the cheap material evaluation in Quiesce as in gnuchess
//                 Eval() takes alpha and beta and returns an estimate if
//                 outside of a window - this needs putting back in SOON!
//                * Have now made a PrintPV() function that prints the current
//                  PV/bestline in the roughly same way as gnuchess(es) does.
//                * Have altered the Null move code back to returning the
//                  score rather than beta to make it work with the nes code,
//                  it may be worth going back and seeing what the extention
//                  after the null move was, as this may now become important.
//                  Also the Deep-Null algorithm may now work IF the PV is
//                  used as it is now - Not sure?
//                  There may be other problems in this version, as little time
//                  has been spent on testing it out - but as far as I can
//                  tell there is a fairly significant speedup from using the
//                  the fail-soft algorithm rather than straight Alpha/Beta PV
//                  search.
//                * Have got the cheap material evals back to be as good as
//                  before.
//                * Have fixed the fail-soft algorithm to work 100% as far as I 
//                  can tell - Played against normal and same scores ect.
//                * Fixed a bug that caused the search result to go wrong
//                  when a time-out occured at the end of the iterations. This
//                  was due to all the extra calls to Search in Think() for
//                  which some had no test for timeout call after them!
//                * Still not sure abount the null move code, have now got a
//                  kind of DEEP-NULL working so/so - but even the normal null
//                  does not seem to work all the time.
//                  There is some problem that causes the lines returned in the
//                  PV to be shorter than IterDepth, this is obviously because
//                  we call Search()/Quiesce() with a smaller depth for null
//                  move, but this does not explain how when a test >Beta is
//                  done, it somehow gets into the PV - Needs looking at soon.
//                * Have sorted out the MoveHistory to work the same as before.
//                * Have got rid of loads of the old commented out code, to
//                  make things clear (see start_again4 for non fail-soft code).
//                * Have also started printing other end of thinking results.
//                * Have finnaly settled on a set up for the null move. I think
//                  we use depth=-3 as we hope to search 2 full plys further
//                  and hence it will make no differnence. Leave as is now, as
//                  it does work and even though using depth=0 seems much faster
//                  it does not significantly improve the performance of the
//                  program perminantly. With depth=0 we miss a lot of moves
//                  later in the search, but search slighly further, it could
//                  be that the extra plys are just crap! LEAVE IT ALONE NOW!
//                * Have also improved the PrintPV() function to print out
//                  in a better way. Now prints checks as a + at end by makeing
//                  the moves in the PV (takes them back after).
//                  Also shows extentions and shallow cuts after PV.
//                * Have made the -NoOutput and -ShowThinking params global so
//                  they may be passed to Search() simply. This now means that
//                  these options work now (no more printing of '&' bounds when
//                  we don't want them.
//                * Have decided not to inc the Fifty move counter in 
//                  MakeNullMove() as this may cause problems. Have decided to
//                  just leave it as it was (rather than 0 it which may cause
//                  even worse problems!). This should be OK.
//                * Have speeded up the TestForRepition() function again by
//                  only looking at states which are 4 (or 2* number of players)
//                  back and four from that ect. This makes a small speed
//                  increase.
//                * Have now implemented the killer move(s) heuristic. One
//                  vector saves old (ie first time set for ever) moves and the
//                  other stores most recent killer moves (this will be similar
//                  to the PV, but no captures or promotions are allowed).
//                  This gains a small but fair increase in speed (15% - 20%).
//                  Could possibly use a sparse version at the cost of about
//                  5mb to 10mb of extra space (may not be wortk the increase!).
// start_again6:  * Have got 2 cheap attack functions working now.
//                * Have mmade the en-passent and castle moves require less
//                  work (may be bugs!).
//                * Have made a slightly faster move generator, but it may be
//                  possible to make one work slightly faster? (ie no struct...)
// start_again7:  * Have fixed 2 bugs: 
//                  - The killer moves had the possiblility of going wrong if
//                    the search went over MAX_SEARCH_DEPTH (31!), but this
//                    never actually happend! - Fixed anyway!
//                  - More importantly (and after a lot of bug hunting!), it
//                    appears that the PV we were printing (and using!) was not
//                    always correct!!! This was due (I think!) to the fact that
//                    when we failed in Search(), we were doing an extra
//                    search with input beta as -X, this meant that we assumed
//                    that what was in the PV was what we wanted! - But this
//                    was not always so. To fix this I just set the input
//                    beta to (-X)+1 in order to force a new copy of the best
//                    ply (ie PV) was saved correctly. This may still not be
//                    100% correct, but it does stop all the problems with
//                    the short (ie less than IterDepth) PV's being printed and
//                    used. It also stops the (barmy!) non-quiescent postions
//                    being used. This made no real speed reduction until
//                    about depth 11/12 where it took longer - possibly because
//                    it took up to depth 9 to go wrong and have a nock on
//                    effect reducing time. Anyway this did not seem to reduce
//                    the playing/search ability of the program at all, but
//                    more it made fake bugs appear when printing out the PV!
//                    THIS NEEDS LOOKING AT AGAIN SOON TO BE 100% SURE IT'S ALL
//                    FIXED OK! - POSSIBLY SOME OF THE WEIRDNESS THAT WAS BLAMED
//                    ON THE NULL MOVE WAS DUE TO THIS!
//                 * Have cludged yet another thing into the interface, that
//                   lets us take back a terminal move (ie: Stale/Check mate).
//                   This is a propper cludge and the whole interface needs
//                   sorting out properly soon - as soon as the way that the
//                   chess engine is going to interface to a standard 
//                   interface - for Windoze aswell ect.
//                   DON'T BOTHER UNTIL SEARCH ENGINE IS FULLY READY!
//                 * Have nearly fully fixed the bug where the PV came out less
//                   than the iter depth. I removed the '+'/'-' stuff to very
//                   little speed losss (if not even faster now!). Also fixed
//                   when the PV was saved (ie after beta cut test) and also
//                   used the same partial fix as above. I have only seen it
//                   do one less (a -2) and that could be down to the null move.
//                   Would be nice to have rid of them all!
//                 * Have got the chess_test.cc program to parse any of the
//                   .fin file (hopefully - just testing now!). The only
//                   problem could arise from the '!?' moves and also from the
//                   fact I'm ignoreing '!' moves and just counting it as
//                   another valid (desired) move. If a botched move or '*'
//                   is found - it is ignored, even so most errors cause a
//                   instant failure.
//                 * Have done several other things to the search mainly, 
//                   includeing testing for beta in a slightly further place.
//                 * Have now made it so that the chess_test.cc program treats
//                   '!?' and '?!' moves as being desired moves as they
//                   are obviously treated as so for the gnuchess tests.
//                   This means that 3 files will need to be re-run for the
//                   new fixed version of the results. 
//                 * Have made it so as soon as a move returns to ply 0 we may 
//                   use this if it's better than the last depths best move 
//                   (this is only possible now we removed the '+'/'-' code in 
//                   Think(). This should help on some problems that I have 
//                   noticed get the correct (partial) answer, but then don't
//                   use it! - It is safe as we always go down the last plys
//                   PV first...
//                   Have introduces the BoundType '%' to signify that we have
//                   only partially searched the ply, also made it so no +/-
//                   is printed for extentions ect for this half searched ply.
//                 * Have got the PrintMove() function to append a '#' rather
//                   that a '+' if the move causes checkmate.
//                 * Have got the jump out of search cause mate has been found
//                   code to work correctly now (wasn't before!). Have also
//                   put the same code in Quiesce() so that it may break when a 
//                   mate is found.
//                 * Have sorted out a few misc bugs to do with MATE-Ply not
//                   working as some were 1 out (eg null move ect - like above).
//                 * Have Got rid of the SortPV() function as it is now sorted
//                   highest in the function GenMoves() and stopped when we
//                   hit the end of it.
//                 * Have added code to sort the capture of the last piece moved
//                   before normal capture. Have added back the code to add
//                   an extra bit of sort score for an capture on promotion.
//                 * Have sorted out Search() and Quiesce() to always goto the
//                   end of the function when leaving, this is so we can store
//                   killers and captures for draws and mates ect. Also by
//                   putting the killer move heuristic in the Quiesce search
//                   we get a significant improvement.
//                   Also there is a clear place to use the transpostion table
//                   now (see comment in both search functions).
//                 * Have added an extra way of using the null move heuristic
//                   called SAFE_NULL, this simply ensures that we don't do
//                   the null move after we fail and need a re-search. This is
//                   the source of all the negative PV's - but it it really
//                   worth using (ie does it play any6 better?).
//                 * Have made the code to print the PV into a function to print
//                   a line - beware as MakeMove could mess things up?
//                 * Have scrapped a bit of useless code that was for the
//                   window. It tested if we failed at ply 0, but ply 0 has
//                   infinite wide beta/alpha now (ie no fail-soft crap)...
//                 * Have added a cotempt for draw option. Genarlly leave it as
//                   0 for testing, but may be more interesting if set later?
//                 * Have added several extra sorting options and this has
//                   helped to speed the code up for tests somewhat.
// start_again8: * Have now goto a transpostion hash table working.
//                 Still PV is rubish and PrintPV with hash is not fail safe,
//                 but there is a large increase of speed from the table.
//                 Still need to get the key genaration sorted out as this is
//                 taking up a large proportion of CPU time and could be done
//                 on the fly.
//                * Have also descovered that GenPush is now taking up the most
//                  CPU time and then Eval() ect (20% for mcount!):
//                   15.18 GenPush()              * Easy to speed up.
//                   8.42 Eval()                  + Un-optimoized as of yet.
//                   8.14 CurrentKey()            * Could be got rid of.
//                   7.35 MakeMove()              - De-Botch would speed up.
//                   5.83 GenCaptures()           - Marginal speedup possible.
//                   5.27 GenMoves()              - Marginal speedup possible.
//                   4.99 Attack()                - Castle and king move cheap.
//                   4.24 QuiesceSearch()         - ?
//                   4.10 Sort()                  - ?
//                   2.66 Search()                - ?
//                   2.56 TestTimeOut()           - Test every N calls?
//                   1.82 EvalPawn()              - Up-optomized, cache them?
//                   1.73 TestExposure()          - ?
//                   1.50 TestNotEnoughMaterial() - ?
//                   1.19 TakeMoveBack()          - No!, simple anyway.
//                   1.07 SingleAttack()          - ?
//                   1.05 EvalKing()              - Up-optomized as of yet.
// start_again9 & start_again10: Not sure? - started a new clean up stage.

// clean_up1: * Started to clean up the code as it's got real messy and needs
//              to be properly sorted out before continuing.
//              Need to not make the code too much slower in the proccess.
//            * Not sure if anything was altered in this version???
// clean_up2: * Have made the old evaluation function into a pure linear 
//              weighted sum now - for easy training (No King Divisor...).
//            * Have made it so the program can train evaulation sets using
//              a simple 'Temporal Differneces' method (Using Delta Rule).
//              This uses the old code, with a new AccessWeight() function
//              which can either read or update a weight for training...
//            * Extra Command line args:
//              -TrainWhite     : Train and asve the white eval set.
//              -TrainBlack     : Train and asve the black eval set.
//              -LearningRate   : Learning rate for training (0.00001 or less?)
//              -NumGames       : Number of games to play in a row (default=1).
//            * Have made the END_GAME_MATERIAL and OPENING_MOVES constants
//              now, rather than parameters as before.
//            * Have updated the old 'hand-crafted' evaluation set to the new 
//              format, so that it may be used as a starting point for training,
//              in order to hopefullt improve on it.
//            * The program will now creat a random eval set if an  invalid name
//              provided and will only same 1 copy if both black and white are 
//              same file.
//            * Have also altered several constants to make training better:
//              - Eval window of 5000.
//              - No null move.
//              - No Hash scores, but new USE_MOVE_ONLY mode for use like a PV.
//              - ... Possibly more?
//            * The training code seems to work OK, even though there are no 
//              terminal reward (only material increase/decrease), so long as 
//              Learning Rate is kept bellow 0.00001, BUT:
//              HAVE CAUSED ABOUT 50% SLOWDOWN OF CODE AS A RESULT OF TRAINING!
//            * Have made it so that when playing multiple games, white and
//              black player eval sets are alternated - so as not to play the
//              same colour for the whole of a training run.
//              NOTE: Not very carefully done - May cause bugs or not work,
//                    especially when not training both colours at a time.
//              Use -Alternate to use this mode...
//                  ^ Found a bug, that caused the opposite eval sets to play!
//            * Have now made it so the opening move masks for the eval will be
//              activated when the total amount of material gets < N or there 
//              are no castling possible for either side.
//              NOTE: This caused the chess_test program to not work properly,
//                    and it was just been using mask opeing masks - But it
//                    now seems worse! WHY?
//            * Have now made it so only quiescent positions will be used
//              for weight updates during training.
// clean_up3: * Have now made it so the eval params are all [3][64].
//              Have got rid of king safty / EvalKing() in the process.
// clean_up4: ??? NOT SURE ??? EVAL LEARNING, POSSIBLY?
// clean_up5: ??? NOT SURE ??? EVAL LEARNING, POSSIBLY?
// clean_up6: * Fixed three bugs in EvalPawn():
//              - A backward pawn was being counted on the back row if it's
//                neighbour(s) had only moved 1 square ahead. This is not so as
//                the pawn may move two squares to make itself non-backward in
//                *1* move - hence it's not backward by definition!
//              - A past pawn utility was being multiplied by how far forward
//                it was. This is NOT needed when using the 64 (Square) features
//                for each features type.
//              - A much worse bug was the fact that the test for pawn 
//                protection for a past pawn, was being executed all the time
//                making the value mean simply: 'pawn protection of a pwan!'.
//                Even so, after fixing the bug a bishop at K/Q's 3rd rank
//                rank sometimes blocks in the pawn at the K/Q's second rank,
//                this may hid that a 'Pawn Protected By Pawn' feature may be
//                useful??? (Along with other protection/defence features).
//                Also a feature may need to be added to say how much blocking
//                in a pawn with a piece of your own is worth (should be -ve!).
//            * Have now made it so the play_game program can play multiple
//              games per run, collecting running stats on wins, loses and draws
//              to collate at the end. This may be used to see how one eval set
//              performs against another (remember to swap white and black!).
//            * Have got rid of the 'Alternate' global along with some weird
//              ES alternating code in Think().
//            * Have now set the material values to be allowed to be trained.
//              This now means that ES has to be set carefully before attempting
//              to use any code that uses the evaluation set loaded.
//              Instead of random settings for training, we use the default 
//              values.
//            * It is now the job of PlayGame() to set ES when playing two 
//              computers players (WHITE=0, BLACK=1).
//              The repective programs/files to set ES as they need it:
//              - play_game.cc: Loads in an evaluation set and uses the
//                              set's material values.
//              - train_eval.cc: Uses both EP[0] and EP[1] for batch updateing.
//                               Set's it's own.
//              - convert_from_pgn.cc: Uses the defealt material values, which
//                                     are loaded in InitAll() if no evaluation
//                                     set has been loaded (as in c_f_pgn.cc!)
//              - chess_test: Sets it's own to EP[0] and uses that for Think().
//            * Have now made the update of weights batch orientated, to stop
//              the fact that the data file may have been (and was!) ordered in
//              some arbitary way (ie: No help to learning process). This 
//              ordering caused a false (low) error to be reported and also 
//              paramter swings during a single pass of the data set.
//              NOTE: Another way to get round this would be to re-randomized
//                    the data-set!
//              As a reult of doing this batch update method, it was found that 
//              the learning rate should be set to *EXACTLY* 1/NumPositions in
//              the data file.
//              LOGIC: If all decide they want a value to go up by 100 (say),
//                     then with a learning rate of 1/N, it WILL go up by
//                     *EXACTLY* 100. If the value was larger, then it may
//                     (and does!) do large oversteps and the learning process 
//                     is ruined. Setting the value smaller simply means we
//                     do not jump as far (ie: with LR=1/2N all saying increase
//                     by 100, the value would only be set to 50!). Even so it
//                     may still be worth while trying lower settings, but
//                     the value should NEVER by larger than 1/N.
//            * Have now made the program report mate will be forced in N+1
//              moves to make easier to understand output. What does X will
//              force mate in 0 moves mean? - Rubish, X will force mate in 1
//              move - ie: The move we have chosen => we mate after the move! 
//            * Have now made it so the fact that a position is quiescent is
//              stored in the data file after the move. This saves a lot of
//              time when training. It seems to work OK, but is a bit of a
//              cludge and realy needs sorting out later. The actual speed 
//              increase is (std base): from 302 seconds per iteration to 70
//              seconds per iteration (x4.3 speedup!).
//            * Have got rid of all 'search' global varialbes and placed them in
//              a structure called 'SearchData'. It contains all stats and
//              search aids ect and saves passing loads to searching functions.
//            * Have now made the MoveStack into a data structure which is held
//              inside of Search() etc (called MoveList). It not is used by
//              the move genertaion code.
//            * Have now made it so CurrentPly is passed as a search function
//              parameter. This saves having it as a global variable.
//            * Have now made it so after the moves have been generated, they
//              are sorted (seperately) inside the Search() functions.
//              This now means that GenMoves() etc are completely Game code and
//              has nothing to do with the search code.
// clean_up7: * Have now sorted the code out into three engines:
//              - ChessEngine      : This is needed by everything.
//              - SearchEngine     : This is needed for searching.
//              - EvaluationEngine : This is needed for evaulation when 
//                                   searching also.
//            * Have renamed many files into better names and reorganized
//              the dirrectory structure.
//            * Have now made the null move part of the search engine code, to
//              get it out of the current state structure.
// clean_up8: * More fidling around with learning...
// clean_up9: * Have now made it so the material values are altered by training.
//              NOTE: No more material sums kept in GameState => Slower!
//              Material functions now used (64 squares checked!!!)
//            * Have added some extra features, but it is very hard to tune
//              them (note: hc_eval.set is *CRAP*.
//            * Have made GetStage() dependant on number of pieces and not 
//              material (could use BASIC MATERIAL VALUES?).
// clean_up10: * Have now made it so that the training uses the hyberbolic
//               tanget (and it's devivative), to saturate the values between
//               -1 and 1. It has been set up so that 100 points (ie: Pawn) is
//               worth 0.25 when passed through the function. Also the function
//               saturates at about 1200 points (Queen + Minor Piece).
//             * Have tried to sort out the KING_PAWN_MOVE and KING_PAWN_STORM,
//               but still they seem to get crazy values?
//             * Have now made it so a passed pawn (or anything else) does not
//               get multiplied (ie: Called in a for loop multiple times).
//               The value is simply counted once.
//             * Have moved the game result constants to the interface code,
//               and not the base chess code.
// clean_up11: * Have now got rid of the move with score structure and have
//               sperated the Sorting scores from the Evaluation score. This
//               will make it much easier to keep Sorting scores as integers
//               and evaluation scores as doubles.
//             * Have now made it so that there are no more material values in 
//               the evaluation set (learning may bias the masks!).
//               This has allowed the running material to be used again, but
//               now it's all held in the SearchData structure and is inited in
//               Think() and updated after each move (including null!) after
//               the call to make move in Search() and QuiescentSearch().
//               NOTE: Training and the QuickSearch functions don't use this,
//                     but use the slower BasicMaterialEval() function
//                     instead! - IE: Only search stuff uses the running values.
//               This caused a 23% (re-)speedup of the code in general!
//             * Have fixed a bug where Search() was calling Quiesce() with
//               CurrentPly+1, this is wrong as we have not made a mode there
//               and should be simply CurrentPly!
//             * Have upped the precision of a pawn to 10000, and have altered
//               all that need changing for this.
//               Have now made it so that only internally (search) the values
//               are 1 pawn = 10000, externally (ie in eval set, screen output
//               and training)  1 pawn is treated as 1 point.
//               This should be enough precision to make the trained eval sets
//               work better, and now a random swing value may be set as low
//               as 0.0001 without loosing precision.
//               NOTE: The random swing now effects at 1 point = 1 pawn!
//             * Have now made it so that the random swing is now a multiplier
//               +/- random N%, rather than a random offset. This is to make it 
//               fairer when testing learnt evaluation functions vs the original
//               one.
//             * Have now made it so that the order of a training data-base is
//               randomized for each iteration.
//             * Have added a new learning rate recution factor etc...
//             * Have scrapped the train online function and made it so that the
//               update is done after training for one game (at end).
//             * Have made it so that DrawBoard now adds the coordinates.
//             * Have made chess_test.cc draw the board with the player to
//               move at the bottom now.
// clean_up12: * Have replaced the Linear Weighted Sum with the BackProp
//               class. This reduces overall speed by 2 when the NN is set up to
//               work like a LWS as before. BUT this does allow the use of
//               hiden layer(s) and more complex outputs (W/L/D...).
//             * Have altered the BackProp class to have new functionsa which
//               use function pointers (mind boglers!) to speed up the input.
//               This is done in a similar way to the old LWS and saves looping
//               through massive feature vectors (6x slower!) and simply gives
//               the job of calculating first layer unit sums for fireing and
//               updateing weights for training function.
//             * In the end it may turn out that extra neurons (which reduce
//               the speed of evaluation by factor N!) may be a waste of time!
//               FUTURE: Logic first layer, Sparse NN, ...?
//               BUGS: May be two in DrawByRep():
//                     - Null move positions are tested and should not be (not 
//                       realy a big problem?).
//                     - The HashKey is not being update for things such as the
//                       player to move and castleing etc. This may effect
//                       'reciprical zugswang' postions (Pretty unlikely!).
// clean_up13: * Have fixed the slowdown bug in QuickQuiesceSearch(), it was 
//               caused by not sorting the moves (MVV-LVA).
//             * Have got rid of all the NN code, and made it back to a simple 
//               LWS - for speed mainly, but also NN didn't realy help!
//             * Have added back the on-the-fly material evaluation code back
//               in! (ie: Add material to postional, pass through atan() for 
//               training / raw for playing).
//             * Have kept certain distance based features, from clean_up13.
//             * Have made it so that the check extention is activated earlier,
//               before the call to quiesce search.
//             * Have added back the aspiration search code, not realy sure if
//               it realy helps. Have also made it so that it is still safe to
//               use the partial search results (by ignoreing fail low/high 
//               moves, and not setting 'ComputersMove' ect until no fail).
// clean_up14: * Have now merged Evaluation and Search engines into Search only.
//             * Have sorted out the TestRep function to now add the castling
//               permisions and the en-passent info to the hash key when 
//               comparing states.
//               NOTE: There was a bug for the repitition check which was 
//                     decrementing the move index by 4, this should be 2 as
//                     repitition's can happen on odd pairs of moves (ie every 
//                     2 moves - same player).
//             * Have got rid of the MakeNullMove function, and made it much
//               simpler - just swap players.
//               NOTE: This should stop any bugs with the draw by repition etc.
//             * Have made the null move set fifty counter back to 0, to help
//               reduce the chance of finding a null move rep draw.
//             * Have made it so training/testing can use a faster 
//               AccessWeight() marcro...
//             * Have added in 5 new pawn features.
// clean_up15b: * Have now made it so there are only 3 stages.
//              * Have now got rid of all but the knight distance based 
//                features.
//              * Have made it so that features may be plotted over time, using
//                plot single program.
// clean_up16: * Have tried to get a correlated version working = poor.
//             * A lot of eval code different now.
//             * Works with new canvas class event interface.
// clean_up17: *** RE-MADE VERSION - LOST WHEN INSTALLING NEW LINUX! ***
//             * Have now made it so the random swing if now an offset (again)
//               rather than a scaler.
//             * Have god rid of all the features, LWS stuff, stages, ... and 
//               have gone back to using a simple (ie: Raw board input) 
//               Backprop network evaluator.
//               - NOTE: Code uses 13 features per square (ie each state a 
//                       square may be in - including NONE!).
//                       Also uses the 'Fast/Sparse' method of using the 
//                       Backprop class. This still means we have 64 input 
//                       opertaions (out of 64*13!) to do and the total ops
//                       being 64*NumHidden unit (for 1 layer!).
//             * Have added the program 'randomize_data_set.cc", which is used 
//               to randomize a *.min data set (as they were ordered before).
//               This is done to stop the training program from using the 
//               ordering to it's advantage (NOTE: Very slow as can't be 
//               buffered!
// clean_up18: * Have added back the 3 stages (NN = 3 times larger input layer).
//             * Have set the target for the NN to +/- 0.95, instead of +/- 1.0.
//             * Have tried to find a better set of (Totalistic) material values
//               for the partitioning of all positions into the 3 stages.
//             * Have sorted out the GetStage() function and its constants, so
//               they don't need changing when the PIECE_VALUES[] change.
//             * Have increased the learning rate by 3 times to make up for the
//               reduced number of training examples in each stage.
//               This is done as now we have EXACTLY 3 times less training 
//               examples in each stage.
//             * HAVE TESTED THE STAGE BASED NN - MARGINALLY BETTER AT TESTS,
//               BUT POORER PLAY IN GENERAL - SCRAPPED!
//             * Have found that the best (featureless) results can be obtained
//               by using: *NO STAGES* and 13 inputs per square. This seems to
//               give the best test results and plays resonably.
// clean_up19: * Have now added back a lot of features, along with the best
//               setup of mask features (ie: 13*6) found in clean_up18.
//             * Have made it so that the (18) features have both a WHITE and
//               BLACK input (like the other im mask features!).

// Things to do in the future:
// ---------------------------
// ** EVERYTHING NEEDS TIDYING UP - Split into files and uses more classes...
// * Sort out the mentioning of check in N moves as mentioned in V11's text.
// * Have a hint function for the user to use.
// * Have Several differnt types of play such as:
//   - Super Agressive : Add for 2N captures.
//   - Agressive       : Add for N captures.
//   - Normal          : Leave value as it is.
//   - Defensive       : Subtract N captures.
//   - Super Defensive : Subtract 2N captures.
//   This could be acomplished by add/subtracting a value from a capture
//   move (Making susre not to be too large a value!).
//   N could be in the range 10 to 25 or somewhere near...
// * Have the ability to set up a board position.
// * Have the ability to load and save a game/posistion.
// * Better Evaluation Function:
//   - Have a value for control of board.
//   - Possibly try to get something to learn it's own eval again?
//   - Score deducted for each square of ithe smae colour as an openents 
//     *LONE* bishop.
//   - Bonus for getting one rook behind the other (on open file etc).
//   - Penalty for not castling or moving king before castling.
// * Better End-Game search/evaluation. 
//   - Possibly use a database or hash-table?
//   - Possibly have special templates/modifiers?
//   Assuming (1200 Material as the minimum peice end game):
//     - King vs King & Queen & Bishop.
//     - King vs King & Queen & Knight.
//     - King vs King & Two Rooks.
//     - King vs King & Queen.
//     - King vs King & Rook & Bishop.
//     - King vs King & Rook & Knight.
//     - King vs King & Two Bishops.
//     - King vs King & Knight & Bishop.
//     - King vs King & Rook.
//     - King vs King & Several Pawns (Self-Suporting, Cordon, King Distances).
//     - King vs King & One Pawn (Cordon, King Distances).
//     ** Others ARE possible such as King & Pawn vs King & ... **
// * Get rid of C stdout functions.
// * Needs a better interface - use X etc (Even only for Chess Board Display).
//   Could use Motif or a Java Interface to a native DLL/excuateble?
//   Could also add sounds/speech for check,mate,castle,etc...
// * Thinking on opponents time / keeping the Primary Variation if predicted
//   move is chosen...
// ?*? Try to get working on the psion using a binary executable and OPL
//     inteface? (Later!!!)

// new ideas (as of start_again7)
// ==============================
// * Use a transpostion table/tree.
//   - Pawn seperate?
//   - Eval cache seperate?
//   - Fields and key generation for hash table or symetry remover for tree
//     will be needed (how?).
// * Better evaulation function (see above, look at other people code ect).
// * Get the other standard tests set going to help.
//   - Possibly even write (decent!) standard move/position parsing code going.
// * Get the (stolen!) opening book working when we have a set of parseing
//   routines for moves...
// * Create a simple end-game data-base for all 4 piece endings if possible.
//   - If not pawns then we can use board symetry to help reduce the required
//     space by a factor of 4ish (to 8?). This will also reduce the total
//     time spent searching the positions ect (may be hard - space limit!).
//   - It may (!) be possible to even do up to 5 or six peices if thought out
//     carefully (just search time is MASSIVE!).
// * Write the (usable!) wrapper code/classes to make the non-speed ensential
//   code much nicer and more reusable.
//   - Think carefully about the classes (that won't slow it down!).
//   - Make it so ANY interface may be joined to the chess playing module using
//     function pointers ect.
//   - Add all the nigling things such as:
//     1. Replay of move (ie take back a take back!).
//     2. Game/position save/load/setup.
//     3. Better 'brain' like file with all computers params in, like eval set
//        but more modular!
//     ... Millions of others too (scab other chess programs options for ideas!)
// * Other new pruning methods?
// * Temporal differance style training of evaluation paramters using back-prop
//   class (or evn special code for this alone!).
// * Sort out all to sensible names and comments ect.
// * Make a makefile for the code to speed up compilation ect and make 
//   conditional compilation simpler using -D pragma options.
// * See if you can scab any data from chessmater (data-bases, book ect). Will
//   need to be a fairly simple format to have a hope!
// * Have another look at move generator and attack functions. If possible
//   make new attack functions for 'CheckIfCastleSquaresAttacked' and
//   'KingMovedCanHeBeAttacked'. The castle one only need check forward moves
//   as can't be attacked on the rank! and the king doen't need to check the
//   rank, file or diagonal he was just on (as he would of been attacked there
//   also!).
// * Sort out the extra search, when a draw is found... Wasteful, use a flag.
//   Also sort out the code that doesn't call Think() if theres only one move
//   to choose (make sure mate in N is printed - use a counter...).
// * Have some other material mates such as king,rook and king,rook ect.
//   May be better to just call a matescan function to be sure it's not a mate!
//   Or even do a reduced search (Quiesce?)...
// * Stop 'Bad-Trades' somehow...

// new_ideas (to encapsulate better... for interfacing)
// ====================================================
// * Create the following classes:
//   - 'ChessEngine'
//   - 'ChessGame'
//   - 'ChessPosition'
//   - 'ChessBoard'
//   - 'ChessMove'
//   - 'ChessPeice'
//   - 'ChessScore'
//     + WhiteToMateIn <int>
//     + BlackToMateIn <int>
//     + GameToDrawIn  <DrawType>
