%-----------------------------------------------------------------------------%
% vim: ft=mercury ts=4 sw=4 et
%-----------------------------------------------------------------------------%
% Copyright (C) 1994-1999, 2003-2007 The University of Melbourne.
% This file may only be copied under the terms of the GNU General
% Public License - see the file COPYING in the Mercury distribution.
%-----------------------------------------------------------------------------%
% 
% File: labelopt.m. 
% Author: zs.
% 
% Module to eliminate useless labels and dead code.
% 
%-----------------------------------------------------------------------------%

:- module ll_backend.labelopt.
:- interface.

:- import_module ll_backend.llds.

:- import_module bool.
:- import_module list.
:- import_module set.

%-----------------------------------------------------------------------------%

    % Build up a set showing which labels are branched to, then traverse the
    % instruction list removing unnecessary labels.  If the instruction before
    % the label branches away, we also remove the instruction block following
    % the label.
    %
:- pred labelopt_main(bool::in, set(label)::in,
    list(instruction)::in, list(instruction)::out, bool::out) is det.

    % Build up a set showing which labels are referred to.  The input set is
    % the list of labels referred to from outside the given list of
    % instructions.
    %
:- pred build_useset(list(instruction)::in, set(label)::in, set(label)::out)
    is det.

%-----------------------------------------------------------------------------%
%-----------------------------------------------------------------------------%

:- implementation.

:- import_module ll_backend.opt_util.

:- import_module maybe.
:- import_module pair.
:- import_module svset.

%-----------------------------------------------------------------------------%

labelopt_main(Final, LayoutLabelSet, Instrs0, Instrs, Mod) :-
    build_useset(Instrs0, LayoutLabelSet, Useset),
    opt_labels_in_instr_list(Instrs0, Instrs1, Useset, Mod),
    (
        Final = yes,
        Mod = yes
    ->
        labelopt_main(Final, LayoutLabelSet, Instrs1, Instrs, _)
    ;
        Instrs = Instrs1
    ).

%-----------------------------------------------------------------------------%

build_useset([], !Useset).
build_useset([Instr | Instructions], !Useset) :-
    Instr = Uinstr - _Comment,
    opt_util.instr_labels(Uinstr, Labels, _CodeAddresses),
    svset.insert_list(Labels, !Useset),
    build_useset(Instructions, !Useset).

%-----------------------------------------------------------------------------%

    % Go through the given instruction sequence. When we find a label, we
    % check whether the label can be branched to either from within the
    % procedure or from the outside. If yes, we leave it alone.  If not, we
    % delete it. We delete the following code as well if the label was
    % preceded by code that cannot fall through.
    %
    % We build up the generated instruction list in reverse order in
    % opt_labels_in_instr_list_2, because building it in right order here
    % would make opt_labels_in_instr_list not tail recursive, and thus unable
    % to handle very long instruction lists.
    %
:- pred opt_labels_in_instr_list(list(instruction)::in, list(instruction)::out,
    set(label)::in, bool::out) is det.

opt_labels_in_instr_list(Instrs0, Instrs, Useset, Mod) :-
    Fallthrough = yes,
    opt_labels_in_instr_list_2(Instrs0, [], RevInstrs, no, Mod, Fallthrough,
        Useset),
    list.reverse(RevInstrs, Instrs).

:- pred opt_labels_in_instr_list_2(list(instruction)::in,
    list(instruction)::in, list(instruction)::out,
    bool::in, bool::out, bool::in, set(label)::in) is det.

opt_labels_in_instr_list_2([], !RevInstrs, !Mod, _Fallthrough, _Useset).
opt_labels_in_instr_list_2([Instr0 | Instrs0], !RevInstrs, !Mod,
        !.Fallthrough, Useset) :-
    Instr0 = Uinstr0 - _Comment,
    ( Uinstr0 = label(Label) ->
        (
            (
                Label = entry_label(EntryType, _),
                ( EntryType = entry_label_exported
                ; EntryType = entry_label_local
                )
            ;
                set.member(Label, Useset)
            )
        ->
            !:RevInstrs = [Instr0 | !.RevInstrs],
            !:Fallthrough = yes
        ;
            eliminate_instr(Instr0, yes(!.Fallthrough), !RevInstrs, !Mod)
        )
    ;
        (
            !.Fallthrough = yes,
            !:RevInstrs = [Instr0 | !.RevInstrs]
        ;
            !.Fallthrough = no,
            eliminate_instr(Instr0, no, !RevInstrs, !Mod)
        ),
        opt_util.can_instr_fall_through(Uinstr0) = Canfallthrough,
        (
            Canfallthrough = yes
        ;
            Canfallthrough = no,
            !:Fallthrough = no
        )
    ),
    opt_labels_in_instr_list_2(Instrs0, !RevInstrs, !Mod,
        !.Fallthrough, Useset).

    % Instead of removing eliminated instructions from the instruction list,
    % we can replace them by placeholder comments. The original comment field
    % on the instruction is often enough to deduce what the eliminated
    % instruction was.
    %
:- pred eliminate_instr(instruction::in, maybe(bool)::in,
    list(instruction)::in, list(instruction)::out,
    bool::in, bool::out) is det.

eliminate_instr(Uinstr0 - Comment0, Label, !RevInstrs, !Mod) :-
    labelopt_eliminate_total(Total),
    (
        Total = yes,
        % We have deleted Uinstr0 by not adding it to !RevInstrs.
        !:Mod = yes
    ;
        Total = no,
        ( Uinstr0 = comment(_) ->
            Uinstr = Uinstr0
        ;
            (
                Label = yes(Follow),
                (
                    Follow = yes,
                    Uinstr = comment("eliminated label only")
                ;
                    Follow = no,
                    Uinstr = comment("eliminated label and block")
                )
            ;
                Label = no,
                Uinstr = comment("eliminated instruction")
            ),
            !:Mod = yes
        ),
        !:RevInstrs = [Uinstr - Comment0 | !.RevInstrs]
    ).

:- pred labelopt_eliminate_total(bool::out) is det.

labelopt_eliminate_total(yes).

%-----------------------------------------------------------------------------%

:- func this_file = string.

this_file = "labelopt.m".

%-----------------------------------------------------------------------------%
:- end_module labelopt.
%-----------------------------------------------------------------------------%
