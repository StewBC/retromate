/*
 *  genPieces.cpp
 *  RetroMate
 *
 *  Created by Oliver Schmidt, January 2020.
 *  Pieces designed by Frank Gebhart, 1980s.
 *
 */

#include <fcntl.h>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

char pieces[] =
    "                     "
    " ****   *****   **** "
    " *  *   *   *   *  * "
    " *  *   *   *   *  * "
    " *  *****   *****  * "
    " *                 * "
    "  *               *  "
    "   *             *   "
    "    * ********* *    "
    "    **         **    "
    "     *         *     "
    "     *         *     "
    "     *         *     "
    "     *         *     "
    "     *         *     "
    "    *           *    "
    "   *             *   "
    "   * *********** *   "
    "  *               *  "
    " ******************* "
    " ******************* "
    "                     "
    "                     "
    " ****   *****   **** "
    " ****   *****   **** "
    " ****   *****   **** "
    " ******************* "
    " ******************* "
    "  *****************  "
    "   ***************   "
    "    *************    "
    "    *           *    "
    "     ***********     "
    "     ***********     "
    "     ***********     "
    "     ***********     "
    "     ***********     "
    "    *************    "
    "   ***************   "
    "   ***************   "
    "  *               *  "
    " ******************* "
    " ******************* "
    "                     "
    "                     "
    "       *   *         "
    "      * * * *        "
    "     *    *  *       "
    "     *        *      "
    "    *          *     "
    "    *   *    * *     "
    "    *        * *     "
    "   *         ** *    "
    "  *           * *    "
    " *              *    "
    " *    ****      *    "
    "  *  *    *   * **   "
    "   **    *    ** *   "
    "        *      *  *  "
    "       *        * *  "
    "      *           ** "
    "     *             * "
    "    * ************ * "
    "    *              * "
    "    **************** "
    "                     "
    "                     "
    "       *   *         "
    "      * * * *        "
    "     ***** ***       "
    "     **********      "
    "    ************     "
    "    **** **** **     "
    "    ********* **     "
    "   **********  **    "
    "  ************ **    "
    " ****************    "
    " ****************    "
    "  ****    **** ***   "
    "   **    *****  **   "
    "        ******* ***  "
    "       ********* **  "
    "      ************** "
    "     *************** "
    "    *              * "
    "    **************** "
    "    **************** "
    "                     "
    "                     "
    "                     "
    "         ***         "
    "        *   *        "
    "       *     *       "
    "      *   *   *      "
    "     *    *    *     "
    "     * ******* *     "
    "    *     *     *    "
    "    *     *     *    "
    "    *     *     *    "
    "     *    *    *     "
    "     *         *     "
    "      *       *      "
    "       *******       "
    "       *     *       "
    "       *     *       "
    "        *****        "
    "     *         *     "
    "   ***************   "
    "  *****************  "
    "                     "
    "                     "
    "                     "
    "         ***         "
    "        *****        "
    "       *******       "
    "      **** ****      "
    "     ***** *****     "
    "     **       **     "
    "    ****** ******    "
    "    ****** ******    "
    "    ****** ******    "
    "     ***** *****     "
    "     ***********     "
    "      *********      "
    "       *     *       "
    "       *******       "
    "       *******       "
    "        *****        "
    "     ***********     "
    "   *             *   "
    "  *****************  "
    "                     "
    "                     "
    "                     "
    "         ***         "
    "        *   *        "
    "   ***  *   *  ***   "
    "  *   *  * *  *   *  "
    " *    * *   * *    * "
    " *   * *     * *   * "
    " *                 * "
    " *   *    *    *   * "
    " *   **  ***  **   * "
    "  *   *********   *  "
    "  *    *******    *  "
    "   *             *   "
    "   *             *   "
    "   * *********** *   "
    "    *           *    "
    "    *           *    "
    "    *           *    "
    "    **         **    "
    "     ***********     "
    "                     "
    "                     "
    "                     "
    "         ***         "
    "        *****        "
    "   ***  *****  ***   "
    "  *****  ***  *****  "
    " ****** ***** ****** "
    " ***** ******* ***** "
    " ******************* "
    " **** **** **** **** "
    " ****  **   **  **** "
    "  ****         ****  "
    "  *****       *****  "
    "   ***************   "
    "   ***************   "
    "   **           **   "
    "    *************    "
    "    *************    "
    "    *************    "
    "    *************    "
    "     ***********     "
    "                     "
    "                     "
    "         ***         "
    "         * *         "
    "       *** ***       "
    "       **   **       "
    "     **  * *  **     "
    "    *  * * * *  *    "
    "   *    ** **    *   "
    "  *      * *      *  "
    "  *  *  *   *  *  *  "
    " *  *    * *    *  * "
    " *  *     *     *  * "
    " *   *    *    *   * "
    "  *   *       *   *  "
    "  *    *     *    *  "
    "   *             *   "
    "   *  *********  *   "
    "    *           *    "
    "    *           *    "
    "    **         **    "
    "     ***********     "
    "                     "
    "                     "
    "         ***         "
    "         * *         "
    "       *** ***       "
    "       **   **       "
    "     **  * *  **     "
    "    **** * * ****    "
    "   ******* *******   "
    "  *******   *******  "
    "  *** ** * * ** ***  "
    " *** ****   **** *** "
    " *** ***** ***** *** "
    " **** ********* **** "
    "  **** ******* ****  "
    "  ***** ***** *****  "
    "   ***************   "
    "   **           **   "
    "    *************    "
    "    *************    "
    "    *************    "
    "     ***********     "
    "                     "
    "                     "
    "                     "
    "                     "
    "                     "
    "                     "
    "         ***         "
    "        *   *        "
    "       *     *       "
    "       *     *       "
    "       *     *       "
    "        *   *        "
    "         ***         "
    "        *   *        "
    "        *   *        "
    "       **   **       "
    "        *   *        "
    "        *   *        "
    "       *******       "
    "      *       *      "
    "     ***********     "
    "     ***********     "
    "                     "
    "                     "
    "                     "
    "                     "
    "                     "
    "                     "
    "         ***         "
    "        *****        "
    "       *******       "
    "       *******       "
    "       *******       "
    "        *****        "
    "         ***         "
    "        *****        "
    "        *****        "
    "       *******       "
    "        *****        "
    "        *****        "
    "       *******       "
    "      *       *      "
    "     ***********     "
    "     ***********     "
    "                     "
    ;

int main(void) {
    int i;
    int f = open("pieces.bin", O_CREAT | O_TRUNC | O_WRONLY);
    char c = 0;

    for (i = 0; i < sizeof(pieces); ++i) {
        c |= (pieces[i] == '*') << i % 7;
        if (i % 7 == 6) {
            write(f, &c, 1);
            c = 0;
        }
    }

    close(f);
    return 0;
}
